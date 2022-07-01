use std::{sync::{Mutex, mpsc::{Sender, self}, mpsc::{Receiver, SyncSender}}, thread, time::Duration};
use lazy_static::lazy_static;
use rand::Rng;
use simplelog::*;
use tonic::{transport::{Server, Channel}, Request, Response, Status};
use eraft_proto::raft_service_client::{RaftServiceClient};
use std::cmp;

use crate::{raft_log::RaftLog, eraft_proto::{self, RequestVoteRequest, RequestVoteResponse, AppendEntriesRequest, AppendEntriesResponse, Entry, EntryType, ApplyMsg}};

// raft node role
#[derive(PartialEq)]
#[derive(Debug, Clone, Copy)]
pub enum NodeRole {
    Follower,
    Candidate,
    Leader,
}

impl Default for NodeRole {
    fn default() -> NodeRole {
        NodeRole::Follower
    }
}

#[derive(Clone)]
#[derive(Debug)]
pub struct Peer {
    pub id: u16,
    pub addr: String,
}

const INIT_TERM: i64 = 0;
const VOTE_FOR_NO_ONE: i16 = -1;

pub trait Raft {

    fn get_leader_id(&self) -> u16;

    fn change_role(&mut self, role: NodeRole);

    fn run_election(&mut self);

    fn tick_run(&mut self);

    fn applier(&mut self);

    fn advance_commit_index_for_leader(&mut self);

    fn advance_commit_index_for_follower(&mut self, leader_commit_id: i64);

    fn propose(&mut self, payload: String) -> (i64, i64, bool);

    fn handle_request_vote(&mut self, req: RequestVoteRequest) -> RequestVoteResponse;

    fn handle_append_enries(&mut self, req: AppendEntriesRequest) -> AppendEntriesResponse;

    fn broadcast_heartbeat(&mut self);

    fn apply_ch(&self) -> &SyncSender<ApplyMsg>;

    fn match_log(&self, term: i64, index: i64) -> bool;

    fn set_apply_ch(&mut self, apply_ch: SyncSender<ApplyMsg>);

}

#[derive(Debug)]
pub struct RaftStack {
    me_id: u16,
    dead: bool,
    apply_ch: SyncSender<ApplyMsg>,
    role: NodeRole,
    cur_term: i64,
    voted_for: i16,
    granted_votes: u16,
    commit_idx: i64,
    logs: RaftLog,
    last_applied: i64,
    next_idx: Vec<i64>,
    match_idx: Vec<i64>,
    is_snapshoting: bool,
    peers: Vec<Peer>,
    leader_id: u16,
    tick_count: u64,
    tick_beat: u64,
    tick_elec: u64,
    election_running: bool,
    heartbeat_running: bool,
    heartbeat_time_base: i64,
    election_timeout: u64,
    election_base: u64
}

impl RaftStack {
    pub fn new(me: u16, peers: Vec<Peer>, heart_base: i64, elec_base: u64, app_apply_ch: SyncSender<ApplyMsg>) -> RaftStack {
        build_raftstack(me, peers, heart_base, elec_base, app_apply_ch)
    }
}

impl Raft for RaftStack {

    fn get_leader_id(&self) -> u16 {
        self.leader_id
    }

    fn match_log(&self, term: i64, index: i64) -> bool {
        return index <= self.logs.get_last().index && self.logs.get_entry(index - self.logs.get_first().index).term as i64 == term;
    }

    fn applier(&mut self) {
        if self.last_applied >= self.commit_idx {
            return;
        }
        let first_index = self.logs.get_first().index;
        let commit_index = self.commit_idx;
        let last_applied = self.last_applied;
        let ents = self.logs.get_range((last_applied + 1 - first_index) as usize, (commit_index + 1 - first_index) as usize);

        simplelog::info!("{:?}, applies entries {:?}-{:?} in term {:?}", self.me_id, self.last_applied, self.commit_idx, self.cur_term);

        for ent in ents {
            let apply_msg = ApplyMsg{
                command_valid: true,
                command: ent.data,
                command_term: ent.term as i64,
                command_index: ent.index,
                snapshot_valid: false,
                snapshot: Vec::new(),
                snapshot_term: 0,
                snapshot_index: 0,
            };
            let _ = self.apply_ch.send(apply_msg);
        }
        self.last_applied = cmp::max(self.last_applied, self.commit_idx);
    }

    fn advance_commit_index_for_follower(&mut self, leader_commit_id: i64) {
        let new_commit_index = cmp::min(leader_commit_id, self.logs.get_last().index);
        if new_commit_index > self.commit_idx {
            simplelog::info!("follower id {:?} advance commit index {:?} at term {:?}", self.me_id, self.commit_idx, self.cur_term);
            self.commit_idx = new_commit_index;
            // TODO: signal notidy apply ch
        }
    }

    fn advance_commit_index_for_leader(&mut self) {
        self.match_idx.sort();
        let n = self.match_idx.len();
        let new_commit_index = self.match_idx[n-(n/2+1)];
        if new_commit_index > self.commit_idx {
            if self.match_log(self.cur_term, new_commit_index) {
                simplelog::info!("leader id {:?} advance commit index {:?} at term {:?}", self.me_id, self.commit_idx, self.cur_term);
                self.commit_idx = new_commit_index;
                // TODO: signal notidy apply ch
            }
        }
    }

    fn propose(&mut self, payload: String) -> (i64, i64, bool) {
        if self.role != NodeRole::Leader {
            return (-1, -1, false);
        }
        simplelog::info!("recv propose {:?}", payload);
        let read_logs = self.logs.clone();
        let last_log = read_logs.get_last();
        let new_log_ent_index = last_log.index + 1;
        let new_log_ent_term = self.cur_term as u64;

        let new_log_ent = Entry{
            entry_type: EntryType::EntryNormal as i32,
            index: new_log_ent_index,
            term: new_log_ent_term,
            data: payload.as_bytes().to_vec(),
        };

        self.logs.append(new_log_ent);
        self.match_idx[self.me_id as usize] = new_log_ent_index;
        self.next_idx[self.me_id as usize] = new_log_ent_index + 1;
        (new_log_ent_index, new_log_ent_term as i64, true)
    }

    fn broadcast_heartbeat(&mut self) {
        let peers_cpy = self.peers.clone();
        for peer in peers_cpy {
            // not send heartbeat for self
            if peer.id == self.me_id {
                continue;
            }   
            if self.role != NodeRole::Leader {
                return;
            }

            let dest_addr = String::from(peer.addr.as_str());
            let rt = tokio::runtime::Runtime::new().unwrap();

            rt.block_on(async{
                let client = RaftServiceClient::connect(dest_addr).await;
                match client {
                    Ok(_) => {
                        simplelog::info!("connect to peer ok");
                        let prev_log_idx = self.next_idx.get(peer.id as usize).unwrap() - 1;
                        simplelog::info!("peer prev_log_index {:?}", prev_log_idx);
                        
                        let first_index = self.logs.get_first().index;
            
                        let mut to_append_entries: Vec<Entry> = Vec::new();
                        let mut from_leader_entries = self.logs.erase_before(prev_log_idx + 1 - first_index);
                        to_append_entries.append(&mut from_leader_entries);
                        let append_size = to_append_entries.len();
            
                        let append_req = tonic::Request::new(
                            AppendEntriesRequest{
                                term: self.cur_term,
                                leader_id: self.leader_id as i64,
                                prev_log_index: prev_log_idx,
                                prev_log_term: self.logs.get_entry(prev_log_idx - first_index).term as i64,
                                leader_commit: self.commit_idx,
                                entries: to_append_entries,
                            }
                        );
                        let resp = client.unwrap().append_entries(append_req).await;
                        match resp {
                            Err(err) => {
                                simplelog::error!("err: {:?}", err)
                            }
                            Ok(response) => {
                                if self.role == NodeRole::Leader {
                                    if response.get_ref().success {
                                        simplelog::info!("send heatbeat to {} success", peer.addr);
                                        self.match_idx[peer.id as usize] = prev_log_idx + append_size as i64;
                                        self.next_idx[peer.id as usize] = self.match_idx[peer.id as usize] + 1;
                                        self.advance_commit_index_for_leader();
                                    } else {
                                        if response.get_ref().term > self.cur_term {
                                            self.change_role(NodeRole::Follower);
                                            self.cur_term = response.get_ref().term;
                                            self.voted_for = VOTE_FOR_NO_ONE;
                                        } else {
                                            self.next_idx[peer.id as usize] = response.get_ref().conflict_index;
                                            if response.get_ref().conflict_index != -1 {
                                                simplelog::info!("deal confict");
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Err(err) => {
                        simplelog::info!("connect err {:?}", err)
                    }
                }
            });

        }
    }

    fn change_role(&mut self, role: NodeRole) {
        if self.role == role {
            return;
        }
        self.role = role;
        simplelog::info!("change role to {}", role_to_string(self.role));
        match role {
            NodeRole::Follower => {
                // stop heartbeat
                self.heartbeat_running = false;
                // reset elec timer
                let mut rng = rand::thread_rng();
                let random_election_timeout = rng.gen_range(self.election_base..2*self.election_base);
                self.election_timeout = random_election_timeout;
                self.tick_elec = 0;
            }
            NodeRole::Candidate => {}
            NodeRole::Leader => {
                let last_log = self.logs.get_last();
                self.leader_id = self.me_id;
                for i in 0..self.peers.len() {
                    self.match_idx[i] = 0;
                    self.next_idx[i] = last_log.index + 1;
                }
                self.election_running = false;
                // reset heatbeat tick
                self.tick_beat = 0;
            }
        }
    }

    fn handle_append_enries(&mut self, req: AppendEntriesRequest) -> AppendEntriesResponse {
        let mut resp = AppendEntriesResponse{
            term: 0,
            conflict_index: 0,
            conflict_term: 0,
            success: false
        };

        if req.term < self.cur_term {
            resp.term = self.cur_term;
            resp.success = false;
            return resp;
        }

        if req.term > self.cur_term {
            self.cur_term = req.term;
            self.voted_for = VOTE_FOR_NO_ONE;
        }

        self.change_role(NodeRole::Follower);
        self.leader_id = req.leader_id as u16;
        // reset elec timer
        let mut rng = rand::thread_rng();
        let random_election_timeout = rng.gen_range(self.election_base..2*self.election_base);
        self.election_timeout = random_election_timeout;
        self.tick_elec = 0;

        if req.prev_log_index < self.logs.get_first().index {
            resp.term = 0;
            resp.success = false;
            simplelog::info!("reject append entries req");
            return resp;
        }

        // TODO: del with confict

        let first_index = self.logs.get_first().index;
        let ents_copy = req.entries.clone();

        let mut index: usize = 0;
        for ent in ents_copy {
            if (ent.index - first_index >= self.logs.size() as i64) || self.logs.get_entry(ent.index - first_index).term != ent.term {
                // TODO: erase after ent.index - first_index
                for new_ent in req.entries.as_slice()[index..].to_vec() {
                    self.logs.append(new_ent);
                }
                break;
            }
            index += 1;
        }

        self.advance_commit_index_for_follower(req.leader_commit);
        resp.term = self.cur_term;
        resp.success = true;
        resp
    }

    fn handle_request_vote(&mut self, req: RequestVoteRequest) -> RequestVoteResponse {
        let mut resp = RequestVoteResponse { term: self.cur_term, vote_granted: false };

        if req.term > self.cur_term || (req.term == self.cur_term && self.voted_for != -1 && self.voted_for != req.candidate_id as i16) {
            resp.term = self.cur_term;
            resp.vote_granted = false;
        }

        if req.term > self.cur_term {
            self.change_role(NodeRole::Follower);
            self.cur_term = req.term;
            self.voted_for = -1;
        }

        let last_log = self.logs.get_last();

        if !(req.last_log_term > last_log.term as i64 || (req.last_log_term == last_log.term as i64 && req.last_log_index >= last_log.index)) {
            resp.term = self.cur_term;
            resp.vote_granted = false;
            return resp;
        }

        self.voted_for = req.candidate_id as i16;
        // reset elec timer
        let mut rng = rand::thread_rng();
        let random_election_timeout = rng.gen_range(self.election_base..2*self.election_base);
        self.election_timeout = random_election_timeout;
        self.tick_elec = 0;
        resp.term = self.cur_term;
        resp.vote_granted = true;
        resp
    }

    fn run_election(&mut self) {
        simplelog::info!("{} start election ", self.me_id);
        // vote for self
        self.granted_votes += 1;
        self.voted_for = self.me_id as i16;
        let peer_size = self.peers.len() as u16;

        let peers_cpy = self.peers.clone();
        for peer in peers_cpy {
            // not send request vote for self
            if peer.id == self.me_id {
                continue;
            }     
                   
            // send rpc
            let dest_addr = String::from(peer.addr.as_str());
            let rt = tokio::runtime::Runtime::new().unwrap();
            // creating a channel ie connection to server
            rt.block_on(async {
                let client = RaftServiceClient::connect(dest_addr).await;
                match client {
                    Ok(_) => {
                        simplelog::info!("connect to peer ok");
                        let req = tonic::Request::new(
                            RequestVoteRequest{
                                term: self.cur_term,
                                candidate_id: self.me_id as i64,
                                last_log_index: self.logs.get_last().index,
                                last_log_term: self.logs.get_last().term as i64,
                            }
                        );
                        let resp = client.unwrap().request_vote(req).await;
                        match resp {
                            Err(err) => {
                                simplelog::error!("err: {:?}", err)
                            }
                            Ok(response) => {
                                if self.role == NodeRole::Candidate {
                                    if response.get_ref().vote_granted {
                                        simplelog::info!("I grant vote");
                                        self.granted_votes += 1;
                                        if self.granted_votes > peer_size / 2 {
                                            simplelog::info!("I win vote");
                                            self.change_role(NodeRole::Leader);
                                            self.granted_votes = 0;
                                        }
                                    } else if response.get_ref().term > self.cur_term {
                                        self.change_role(NodeRole::Follower);
                                        self.cur_term = response.get_ref().term;
                                        self.voted_for = -1;
                                        // TODO: persis state
                                    }
                                }
                            }
                        }
                    }
                    Err(err) => {
                        simplelog::info!("connect err {:?}", err)
                    }
                }
            });
    
        }
    }

    fn tick_run(&mut self) {
        self.tick_count += 1;
        self.tick_beat += 1;
        self.tick_elec += 1;
        if self.tick_beat == self.heartbeat_time_base as u64 {
            if self.heartbeat_running {
                self.broadcast_heartbeat();
                // reset heartbeat timer
                self.tick_beat = 0;
            }
        }
        if self.tick_elec == self.election_timeout {
            if self.election_running {
                self.change_role(NodeRole::Candidate);
                self.cur_term += 1;
                self.run_election();
                // reset election timer
                let mut rng = rand::thread_rng();
                let random_election_timeout = rng.gen_range(self.election_base..2*self.election_base);
                self.election_timeout = random_election_timeout;
                self.tick_elec = 0;
            }
        }

        simplelog::info!("tick count {:?}", self.tick_count);
        if self.election_running {
            simplelog::info!("elec count {:?}", self.tick_elec);
        }
    }

    fn apply_ch(&self) -> &SyncSender<ApplyMsg> {
        &self.apply_ch
    }

    fn set_apply_ch(&mut self, apply_ch: SyncSender<ApplyMsg>) {
        self.apply_ch = apply_ch;
    }
}

fn build_raftstack(me: u16, prs: Vec<Peer>, heart_base: i64, elec_base: u64, app_apply_ch: SyncSender<ApplyMsg>) -> RaftStack {
    let mut rng = rand::thread_rng();
    let random_election_timeout = rng.gen_range(elec_base..2*elec_base);
    let peer_size = prs.len();
    simplelog::info!("raft gen random timeout {}", random_election_timeout);
    RaftStack {
        me_id: me, 
        dead: false, 
        apply_ch: app_apply_ch, 
        role: NodeRole::Follower, 
        cur_term: INIT_TERM, 
        voted_for: VOTE_FOR_NO_ONE, 
        granted_votes: 0, 
        commit_idx: 0, 
        logs: RaftLog::new(),
        last_applied: 0, 
        peers: prs,
        next_idx: vec![0; peer_size], 
        match_idx: vec![0; peer_size], 
        is_snapshoting: false, 
        leader_id: 0, 
        tick_count: 0,
        heartbeat_running: true,
        election_running: true,
        heartbeat_time_base: heart_base, 
        tick_beat: 0,
        tick_elec: 0,
        election_timeout: random_election_timeout,
        election_base: elec_base
    }
}

// role type to string
fn role_to_string(role: NodeRole) -> String {
    match role {
        NodeRole::Follower => {
            let r = String::from("follower");
            r
        }
        NodeRole::Candidate => {
            let r = String::from("candidate");
            r
        }
        NodeRole::Leader => {
            let r = String::from("leader");
            r
        }
    }
}


#[cfg(test)]
mod raftcore_tests {
    use std::sync::mpsc;
    use std::thread;

    use crate::{raftcore::{INIT_TERM, VOTE_FOR_NO_ONE, Peer}, eraft_proto::raft_service_client::RaftServiceClient};

    use super::{role_to_string, NodeRole, build_raftstack};

    #[test]
    fn test_role_to_str() {
        let role_leader = NodeRole::Leader;
        let res = role_to_string(role_leader);
        assert_eq!(String::from("leader"), res);
        let role_candidate = NodeRole::Candidate;
        let res = role_to_string(role_candidate);
        assert_eq!(String::from("candidate"), res);
        let role_follower = NodeRole::Follower;
        let res = role_to_string(role_follower);
        assert_eq!(String::from("follower"), res);
    }

    #[test]
    fn test_build_raftstack() {
        let peers : Vec<Peer> = vec![
            Peer{
                id: 0,
                addr: String::from("127.0.0.1:8088")
            },
            Peer{
                id: 1,
                addr: String::from("127.0.0.1:8089")
            },
            Peer{
                id: 2,
                addr: String::from("127.0.0.1:8090")
            }           
        ];
        let (tx, rx) = mpsc::sync_channel(2);
        let tx1 = tx.clone();

        let raftstack = build_raftstack(1, peers, 1, 5, tx);
        assert_eq!(raftstack.me_id, 1);
        assert_eq!(raftstack.heartbeat_time_base, 1);
        assert!(raftstack.election_timeout >= 5);
        assert_eq!(raftstack.cur_term, INIT_TERM);
        assert_eq!(raftstack.voted_for, VOTE_FOR_NO_ONE);
        assert_eq!(raftstack.next_idx.len(), 3);
        assert_eq!(raftstack.match_idx.len(), 3);
        assert_eq!(raftstack.granted_votes, 0);
        assert_eq!(raftstack.last_applied, 0);
        assert_eq!(raftstack.leader_id, 0);
        assert_eq!(raftstack.dead, false);
        assert_eq!(raftstack.commit_idx, 0);
        let my_role = NodeRole::Follower;
        assert_eq!(raftstack.role, my_role);
        assert_eq!(raftstack.is_snapshoting, false);

        thread::spawn(move || {
            let val = String::from("hi eraft!");
            tx1.send(val).unwrap();
        });

        let received = rx.recv().unwrap();
        println!("Got: {}", received);

        // test log
        assert_eq!(raftstack.logs.size(), 1);

    }

}
