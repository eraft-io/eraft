//
// MIT License

// Copyright (c) 2022 eraft dev group

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

package raftcore

import (
	"context"
	"sort"
	"sync"
	"sync/atomic"
	"time"

	"github.com/eraft-io/eraft/logger"
	pb "github.com/eraft-io/eraft/raftpb"
	storage_eng "github.com/eraft-io/eraft/storage"
)

type NodeRole uint8

// raft node state
const (
	NodeRoleFollower NodeRole = iota
	NodeRoleCandidate
	NodeRoleLeader
)

func NodeToString(role NodeRole) string {
	switch role {
	case NodeRoleCandidate:
		return "Candidate"
	case NodeRoleFollower:
		return "Follower"
	case NodeRoleLeader:
		return "Leader"
	}
	return "unknow"
}

// raft stack definition
type Raft struct {
	mu             sync.RWMutex
	peers          []*RaftPeerNode // rpc client end
	id             int64
	dead           int32
	applyCh        chan *pb.ApplyMsg
	applyCond      *sync.Cond
	replicatorCond []*sync.Cond
	role           NodeRole
	curTerm        int64
	votedFor       int64
	grantedVotes   int
	logs           *RaftLog
	commitIdx      int64
	lastApplied    int64
	nextIdx        []int
	matchIdx       []int
	isSnapshoting  bool

	leaderId         int64
	electionTimer    *time.Timer
	heartbeatTimer   *time.Timer
	heartBeatTimeout uint64
	baseElecTimeout  uint64
}

func MakeRaft(peers []*RaftPeerNode, me int64, newdbEng storage_eng.KvStore, applyCh chan *pb.ApplyMsg, heartbeatTimeOutMs uint64, baseElectionTimeOutMs uint64) *Raft {
	rf := &Raft{
		peers:            peers,
		id:               me,
		dead:             0,
		applyCh:          applyCh,
		replicatorCond:   make([]*sync.Cond, len(peers)),
		role:             NodeRoleFollower,
		curTerm:          0,
		votedFor:         VOTE_FOR_NO_ONE,
		grantedVotes:     0,
		isSnapshoting:    false,
		logs:             MakePersistRaftLog(newdbEng),
		nextIdx:          make([]int, len(peers)),
		matchIdx:         make([]int, len(peers)),
		heartbeatTimer:   time.NewTimer(time.Millisecond * time.Duration(heartbeatTimeOutMs)),
		electionTimer:    time.NewTimer(time.Millisecond * time.Duration(MakeAnRandomElectionTimeout(int(baseElectionTimeOutMs)))),
		baseElecTimeout:  baseElectionTimeOutMs,
		heartBeatTimeout: heartbeatTimeOutMs,
	}
	rf.curTerm, rf.votedFor = rf.logs.ReadRaftState()
	rf.applyCond = sync.NewCond(&rf.mu)
	last_log := rf.logs.GetLast()
	for _, peer := range peers {
		logger.ELogger().Sugar().Debugf("peer addr %s id %d", peer.addr, peer.id)
		rf.matchIdx[peer.id], rf.nextIdx[peer.id] = 0, int(last_log.Index+1)
		if int64(peer.id) != me {
			rf.replicatorCond[peer.id] = sync.NewCond(&sync.Mutex{})
			go rf.Replicator(peer)
		}
	}

	go rf.Tick()

	go rf.Applier()

	return rf
}

func (rf *Raft) PersistRaftState() {
	rf.logs.PersistRaftState(rf.curTerm, rf.votedFor)
}

func (rf *Raft) Kill() {
	atomic.StoreInt32(&rf.dead, 1)
}

func (rf *Raft) IsKilled() bool {
	return atomic.LoadInt32(&rf.dead) == 1
}

func (rf *Raft) SwitchRaftNodeRole(role NodeRole) {
	if rf.role == role {
		return
	}
	rf.role = role
	logger.ELogger().Sugar().Debugf("node change role to -> %s \n", NodeToString(role))
	switch role {
	case NodeRoleFollower:
		rf.heartbeatTimer.Stop()
		rf.electionTimer.Reset(time.Duration(MakeAnRandomElectionTimeout(int(rf.baseElecTimeout))) * time.Millisecond)
	case NodeRoleCandidate:
	case NodeRoleLeader:
		// become leader，set replica (matchIdx and nextIdx) processs table
		lastLog := rf.logs.GetLast()
		rf.leaderId = int64(rf.id)
		for i := 0; i < len(rf.peers); i++ {
			rf.matchIdx[i], rf.nextIdx[i] = 0, int(lastLog.Index+1)
		}
		rf.electionTimer.Stop()
		rf.heartbeatTimer.Reset(time.Duration(rf.heartBeatTimeout) * time.Millisecond)
	}
}

func (rf *Raft) IncrCurrentTerm() {
	rf.curTerm += 1
}

func (rf *Raft) GetState() (int, bool) {
	rf.mu.RLock()
	defer rf.mu.RUnlock()
	return int(rf.curTerm), rf.role == NodeRoleLeader
}

func (rf *Raft) IncrGrantedVotes() {
	rf.grantedVotes += 1
}

func (rf *Raft) ReInitLog() {
	rf.logs.ReInitLogs()
}

// HandleRequestVote  handle request vote from other node
func (rf *Raft) HandleRequestVote(req *pb.RequestVoteRequest, resp *pb.RequestVoteResponse) {
	rf.mu.Lock()
	defer rf.mu.Unlock()
	defer rf.PersistRaftState()

	if req.Term < rf.curTerm || (req.Term == rf.curTerm && rf.votedFor != -1 && rf.votedFor != req.CandidateId) {
		resp.Term, resp.VoteGranted = rf.curTerm, false
		return
	}

	if req.Term > rf.curTerm {
		rf.SwitchRaftNodeRole(NodeRoleFollower)
		rf.curTerm, rf.votedFor = req.Term, -1
	}

	last_log := rf.logs.GetLast()

	if req.LastLogTerm < int64(last_log.Term) || (req.LastLogTerm == int64(last_log.Term) && req.LastLogIndex < last_log.Index) {
		resp.Term, resp.VoteGranted = rf.curTerm, false
		return
	}

	rf.votedFor = req.CandidateId
	rf.electionTimer.Reset(time.Millisecond * time.Duration(MakeAnRandomElectionTimeout(int(rf.baseElecTimeout))))
	resp.Term, resp.VoteGranted = rf.curTerm, true
}

//
// HandleRequestVote  handle append entries from other node
//

func (rf *Raft) GetLeaderId() int64 {
	rf.mu.RLock()
	defer rf.mu.RUnlock()
	return rf.leaderId
}

func (rf *Raft) HandleAppendEntries(req *pb.AppendEntriesRequest, resp *pb.AppendEntriesResponse) {
	rf.mu.Lock()
	defer rf.mu.Unlock()
	defer rf.PersistRaftState()

	if req.Term < rf.curTerm {
		resp.Term = rf.curTerm
		resp.Success = false
		return
	}

	if req.Term > rf.curTerm {
		rf.curTerm = req.Term
		rf.votedFor = VOTE_FOR_NO_ONE
	}

	rf.SwitchRaftNodeRole(NodeRoleFollower)
	rf.leaderId = req.LeaderId
	rf.electionTimer.Reset(time.Millisecond * time.Duration(MakeAnRandomElectionTimeout(int(rf.baseElecTimeout))))

	if req.PrevLogIndex < int64(rf.logs.GetFirst().Index) {
		resp.Term = 0
		resp.Success = false
		logger.ELogger().Sugar().Debugf("peer %d reject append entires request from %d", rf.id, req.LeaderId)
		return
	}

	if !rf.MatchLog(req.PrevLogTerm, req.PrevLogIndex) {
		resp.Term = rf.curTerm
		resp.Success = false
		last_index := rf.logs.GetLast().Index
		if last_index < req.PrevLogIndex {
			logger.ELogger().Sugar().Warnf("log confict with term %d, index %d", -1, last_index+1)
			resp.ConflictTerm = -1
			resp.ConflictIndex = last_index + 1
		} else {
			first_index := rf.logs.GetFirst().Index
			resp.ConflictTerm = int64(rf.logs.GetEntry(req.PrevLogIndex).Term)
			index := req.PrevLogIndex - 1
			for index >= int64(first_index) && rf.logs.GetEntry(index).Term == uint64(resp.ConflictTerm) {
				index--
			}
			resp.ConflictIndex = index
		}
		return
	}

	first_index := rf.logs.GetFirst().Index
	for index, entry := range req.Entries {
		if int(entry.Index-first_index) >= rf.logs.LogItemCount() || rf.logs.GetEntry(entry.Index).Term != entry.Term {
			rf.logs.EraseAfter(entry.Index-first_index, true)
			for _, newEnt := range req.Entries[index:] {
				rf.logs.Append(newEnt)
			}
			break
		}
	}

	rf.advanceCommitIndexForFollower(int(req.LeaderCommit))
	resp.Term = rf.curTerm
	resp.Success = true
}

func (rf *Raft) CondInstallSnapshot(lastIncluedTerm int, lastIncludedIndex int, snapshot []byte) bool {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	if lastIncludedIndex <= int(rf.commitIdx) {
		return false
	}

	if lastIncludedIndex > int(rf.logs.GetLast().Index) {
		rf.logs.ReInitLogs()
	} else {
		rf.logs.EraseBefore(int64(lastIncludedIndex), true)
		rf.logs.SetEntFirstData([]byte{})
	}
	// update dummy entry with lastIncludedTerm and lastIncludedIndex
	rf.logs.ResetFirstEntryTermAndIndex(int64(lastIncluedTerm), int64(lastIncludedIndex))

	rf.lastApplied = int64(lastIncludedIndex)
	rf.commitIdx = int64(lastIncludedIndex)

	return true
}

// take a snapshot
func (rf *Raft) Snapshot(index int, snapshot []byte) {
	rf.mu.Lock()
	defer rf.mu.Unlock()
	rf.isSnapshoting = true
	snapshot_index := rf.logs.GetFirstLogId()
	if index <= int(snapshot_index) {
		rf.isSnapshoting = false
		logger.ELogger().Sugar().Warnf("reject snapshot, current snapshotIndex is larger in cur term")
		return
	}
	rf.logs.EraseBefore(int64(index), true)
	rf.logs.SetEntFirstData([]byte{})
	logger.ELogger().Sugar().Debugf("del log entry before idx %d", index)
	rf.isSnapshoting = false
	rf.logs.PersisSnapshot(snapshot)
}

func (rf *Raft) ReadSnapshot() []byte {
	b, err := rf.logs.ReadSnapshot()
	if err != nil {
		logger.ELogger().Sugar().Error(err.Error())
	}
	return b
}

// install snapshot from leader
func (rf *Raft) HandleInstallSnapshot(request *pb.InstallSnapshotRequest, response *pb.InstallSnapshotResponse) {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	response.Term = rf.curTerm

	if request.Term < rf.curTerm {
		return
	}

	if request.Term > rf.curTerm {
		rf.curTerm = request.Term
		rf.votedFor = -1
		rf.PersistRaftState()
	}

	rf.SwitchRaftNodeRole(NodeRoleFollower)
	rf.electionTimer.Reset(time.Millisecond * time.Duration(MakeAnRandomElectionTimeout(int(rf.baseElecTimeout))))

	if request.LastIncludedIndex <= rf.commitIdx {
		return
	}

	go func() {
		rf.applyCh <- &pb.ApplyMsg{
			SnapshotValid: true,
			Snapshot:      request.Data,
			SnapshotTerm:  request.LastIncludedTerm,
			SnapshotIndex: request.LastIncludedIndex,
		}
	}()

}

func (rf *Raft) GetLogCount() int {
	rf.mu.Lock()
	defer rf.mu.Unlock()
	return rf.logs.LogItemCount()
}

func (rf *Raft) advanceCommitIndexForLeader() {
	sort.Ints(rf.matchIdx)
	n := len(rf.matchIdx)
	// [18 18 '19 19 20] majority replicate log index 19
	// [18 '18 19] majority replicate log index 18
	// [18 '18 19 20] majority replicate log index 18
	new_commit_index := rf.matchIdx[n-(n/2+1)]
	if new_commit_index > int(rf.commitIdx) {
		if rf.MatchLog(rf.curTerm, int64(new_commit_index)) {
			logger.ELogger().Sugar().Debugf("leader advance commit lid %d index %d at term %d", rf.id, rf.commitIdx, rf.curTerm)
			rf.commitIdx = int64(new_commit_index)
			rf.applyCond.Signal()
		}
	}
}

func (rf *Raft) advanceCommitIndexForFollower(leaderCommit int) {
	new_commit_index := Min(leaderCommit, int(rf.logs.GetLast().Index))
	if new_commit_index > int(rf.commitIdx) {
		logger.ELogger().Sugar().Debugf("peer %d advance commit index %d at term %d", rf.id, rf.commitIdx, rf.curTerm)
		rf.commitIdx = int64(new_commit_index)
		rf.applyCond.Signal()
	}
}

// MatchLog is log matched
func (rf *Raft) MatchLog(term, index int64) bool {
	return index <= int64(rf.logs.GetLast().Index) && rf.logs.GetEntry(index).Term == uint64(term)
}

// Election  make a new election
func (rf *Raft) Election() {
	logger.ELogger().Sugar().Debugf("%d start election ", rf.id)

	rf.IncrGrantedVotes()
	rf.votedFor = int64(rf.id)
	vote_req := &pb.RequestVoteRequest{
		Term:         rf.curTerm,
		CandidateId:  int64(rf.id),
		LastLogIndex: int64(rf.logs.GetLast().Index),
		LastLogTerm:  int64(rf.logs.GetLast().Term),
	}
	rf.PersistRaftState()
	for _, peer := range rf.peers {
		if int64(peer.id) == rf.id {
			continue
		}
		go func(peer *RaftPeerNode) {
			logger.ELogger().Sugar().Debugf("send request vote to %s %s", peer.addr, vote_req.String())

			request_vote_resp, err := (*peer.raftServiceCli).RequestVote(context.Background(), vote_req)
			if err != nil {
				logger.ELogger().Sugar().Errorf("send request vote to %s failed %v", peer.addr, err.Error())
			}
			if request_vote_resp != nil {
				rf.mu.Lock()
				defer rf.mu.Unlock()
				logger.ELogger().Sugar().Errorf("send request vote to %s recive -> %s, curterm %d, req term %d", peer.addr, request_vote_resp.String(), rf.curTerm, vote_req.Term)
				if rf.curTerm == vote_req.Term && rf.role == NodeRoleCandidate {
					if request_vote_resp.VoteGranted {
						// success granted the votes
						rf.IncrGrantedVotes()
						if rf.grantedVotes > len(rf.peers)/2 {
							logger.ELogger().Sugar().Debugf("I'm win this term, (node %d) get majority votes int term %d ", rf.id, rf.curTerm)
							rf.SwitchRaftNodeRole(NodeRoleLeader)
							rf.BroadcastHeartbeat()
							rf.grantedVotes = 0
						}
					} else if request_vote_resp.Term > rf.curTerm {
						// request vote reject
						rf.SwitchRaftNodeRole(NodeRoleFollower)
						rf.curTerm, rf.votedFor = request_vote_resp.Term, -1
						rf.PersistRaftState()
					}
				}
			}
		}(peer)
	}
}

//
// BroadcastAppend broadcast append to peers
//

func (rf *Raft) BroadcastAppend() {
	for _, peer := range rf.peers {
		if peer.id == uint64(rf.id) {
			continue
		}
		rf.replicatorCond[peer.id].Signal()
	}
}

// BroadcastHeartbeat broadcast heartbeat to peers
func (rf *Raft) BroadcastHeartbeat() {
	for _, peer := range rf.peers {
		if int64(peer.id) == rf.id {
			continue
		}
		logger.ELogger().Sugar().Debugf("send heart beat to %s", peer.addr)
		go func(peer *RaftPeerNode) {
			rf.replicateOneRound(peer)
		}(peer)
	}
}

// Tick raft heart, this ticket trigger raft main flow running
func (rf *Raft) Tick() {
	for !rf.IsKilled() {
		select {
		case <-rf.electionTimer.C:
			{
				rf.SwitchRaftNodeRole(NodeRoleCandidate)
				rf.IncrCurrentTerm()
				rf.Election()
				rf.electionTimer.Reset(time.Millisecond * time.Duration(MakeAnRandomElectionTimeout(int(rf.baseElecTimeout))))
			}
		case <-rf.heartbeatTimer.C:
			{
				if rf.role == NodeRoleLeader {
					rf.BroadcastHeartbeat()
					rf.heartbeatTimer.Reset(time.Millisecond * time.Duration(rf.heartBeatTimeout))
				}
			}
		}
	}
}

//
// Propose the interface to the appplication propose a operation
//

func (rf *Raft) Propose(payload []byte) (int, int, bool) {
	rf.mu.Lock()
	defer rf.mu.Unlock()
	if rf.role != NodeRoleLeader {
		return -1, -1, false
	}
	if rf.isSnapshoting {
		return -1, -1, false
	}
	newLog := rf.Append(payload)
	rf.BroadcastAppend()
	return int(newLog.Index), int(newLog.Term), true
}

//
// Append append a new command to it's logs
//

func (rf *Raft) Append(command []byte) *pb.Entry {
	lastLog := rf.logs.GetLast()
	newLog := &pb.Entry{
		Index: lastLog.Index + 1,
		Term:  uint64(rf.curTerm),
		Data:  command,
	}
	rf.logs.Append(newLog)
	rf.matchIdx[rf.id] = int(newLog.Index)
	rf.nextIdx[rf.id] = int(newLog.Index) + 1
	rf.PersistRaftState()
	return newLog
}

// CloseEndsConn close rpc client connect
func (rf *Raft) CloseEndsConn() {
	for _, peer := range rf.peers {
		peer.CloseAllConn()
	}
}

// Replicator manager duplicate run
func (rf *Raft) Replicator(peer *RaftPeerNode) {
	rf.replicatorCond[peer.id].L.Lock()
	defer rf.replicatorCond[peer.id].L.Unlock()
	for !rf.IsKilled() {
		logger.ELogger().Sugar().Debug("peer id wait for replicating...")
		for !(rf.role == NodeRoleLeader && rf.matchIdx[peer.id] < int(rf.logs.GetLast().Index)) {
			rf.replicatorCond[peer.id].Wait()
		}
		rf.replicateOneRound(peer)
	}
}

// replicateOneRound duplicate log entries to other nodes in the cluster
func (rf *Raft) replicateOneRound(peer *RaftPeerNode) {
	rf.mu.RLock()
	if rf.role != NodeRoleLeader {
		rf.mu.RUnlock()
		return
	}
	prev_log_index := uint64(rf.nextIdx[peer.id] - 1)
	logger.ELogger().Sugar().Debugf("leader prev log index %d", prev_log_index)
	if prev_log_index < uint64(rf.logs.GetFirst().Index) {
		first_log := rf.logs.GetFirst()
		snap_shot_req := &pb.InstallSnapshotRequest{
			Term:              rf.curTerm,
			LeaderId:          int64(rf.id),
			LastIncludedIndex: first_log.Index,
			LastIncludedTerm:  int64(first_log.Term),
			Data:              rf.ReadSnapshot(),
		}

		rf.mu.RUnlock()

		logger.ELogger().Sugar().Debugf("send snapshot to %s with %s", peer.addr, snap_shot_req.String())

		snapshot_resp, err := (*peer.raftServiceCli).Snapshot(context.Background(), snap_shot_req)
		if err != nil {
			logger.ELogger().Sugar().Errorf("send snapshot to %s failed %v", peer.addr, err.Error())
		}

		rf.mu.Lock()
		logger.ELogger().Sugar().Debugf("send snapshot to %s with resp %s", peer.addr, snapshot_resp.String())

		if snapshot_resp != nil {
			if rf.role == NodeRoleLeader && rf.curTerm == snap_shot_req.Term {
				if snapshot_resp.Term > rf.curTerm {
					rf.SwitchRaftNodeRole(NodeRoleFollower)
					rf.curTerm = snapshot_resp.Term
					rf.votedFor = -1
					rf.PersistRaftState()
				} else {
					logger.ELogger().Sugar().Debugf("set peer %d matchIdx %d", peer.id, snap_shot_req.LastIncludedIndex)
					rf.matchIdx[peer.id] = int(snap_shot_req.LastIncludedIndex)
					rf.nextIdx[peer.id] = int(snap_shot_req.LastIncludedIndex) + 1
				}
			}
		}
		rf.mu.Unlock()
	} else {
		first_index := rf.logs.GetFirst().Index
		logger.ELogger().Sugar().Debugf("first log index %d", first_index)
		new_ents, _ := rf.logs.EraseBefore(int64(prev_log_index)+1, false)
		entries := make([]*pb.Entry, len(new_ents))
		copy(entries, new_ents)

		append_ent_req := &pb.AppendEntriesRequest{
			Term:         rf.curTerm,
			LeaderId:     int64(rf.id),
			PrevLogIndex: int64(prev_log_index),
			PrevLogTerm:  int64(rf.logs.GetEntry(int64(prev_log_index)).Term),
			Entries:      entries,
			LeaderCommit: rf.commitIdx,
		}
		rf.mu.RUnlock()

		// send empty ae to peers
		resp, err := (*peer.raftServiceCli).AppendEntries(context.Background(), append_ent_req)
		if err != nil {
			logger.ELogger().Sugar().Errorf("send append entries to %s failed %v\n", peer.addr, err.Error())
		}
		if rf.role == NodeRoleLeader && rf.curTerm == append_ent_req.Term {
			if resp != nil {
				// deal with appendRnt resp
				if resp.Success {
					logger.ELogger().Sugar().Debugf("send heart beat to %s success", peer.addr)
					rf.matchIdx[peer.id] = int(append_ent_req.PrevLogIndex) + len(append_ent_req.Entries)
					rf.nextIdx[peer.id] = rf.matchIdx[peer.id] + 1
					rf.advanceCommitIndexForLeader()
				} else {
					// there is a new leader in group
					if resp.Term > rf.curTerm {
						rf.SwitchRaftNodeRole(NodeRoleFollower)
						rf.curTerm = resp.Term
						rf.votedFor = VOTE_FOR_NO_ONE
						rf.PersistRaftState()
					} else if resp.Term == rf.curTerm {
						rf.nextIdx[peer.id] = int(resp.ConflictIndex)
						if resp.ConflictTerm != -1 {
							for i := append_ent_req.PrevLogIndex; i >= int64(first_index); i-- {
								if rf.logs.GetEntry(i).Term == uint64(resp.ConflictTerm) {
									rf.nextIdx[peer.id] = int(i + 1)
									break
								}
							}
						}
					}
				}
			}
		}
	}
}

// Applier() Write the commited message to the applyCh channel
// and update lastApplied
func (rf *Raft) Applier() {
	for !rf.IsKilled() {
		rf.mu.Lock()
		for rf.lastApplied >= rf.commitIdx {
			logger.ELogger().Sugar().Debug("applier ...")
			rf.applyCond.Wait()
		}

		commit_index, last_applied := rf.commitIdx, rf.lastApplied
		entries := make([]*pb.Entry, commit_index-last_applied)
		copy(entries, rf.logs.GetRange(last_applied+1, commit_index))
		logger.ELogger().Sugar().Debugf("%d, applies entries %d-%d in term %d", rf.id, rf.lastApplied, commit_index, rf.curTerm)

		rf.mu.Unlock()
		for _, entry := range entries {
			rf.applyCh <- &pb.ApplyMsg{
				CommandValid: true,
				Command:      entry.Data,
				CommandTerm:  int64(entry.Term),
				CommandIndex: int64(entry.Index),
			}
		}

		rf.mu.Lock()
		rf.lastApplied = int64(Max(int(rf.lastApplied), int(commit_index)))
		rf.mu.Unlock()
	}
}
