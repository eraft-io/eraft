// Copyright [2022] [WellWood] [wellwood-x@googlegroups.com]

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

// 	http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package raft

import (
	"context"
	"fmt"
	"sort"
	"sync"
	"sync/atomic"
	"time"

	"github.com/eraft-io/eraft/pkg/log"
	pb "github.com/eraft-io/eraft/pkg/protocol"
)

type RAFTROLE uint8

const None int64 = 0

//
// raft node stateim
//
const (
	FOLLOWER RAFTROLE = iota
	CANDIDATE
	LEADER
)

func RoleToString(role RAFTROLE) string {
	switch role {
	case CANDIDATE:
		return "Candidate"
	case FOLLOWER:
		return "Follower"
	case LEADER:
		return "Leader"
	}
	return "unknow"
}

//
// raft stack definition
//
type Raft struct {
	mu             sync.RWMutex
	peers          []*RaftClientEnd // rpc client end
	me             int
	dead           int32
	applyCh        chan *pb.ApplyMsg
	applyCond      *sync.Cond
	replicatorCond []*sync.Cond
	role           RAFTROLE

	curTerm      int64
	votedFor     int64
	grantedVotes int
	logs         *RaftLog

	commitIdx     int64
	lastApplied   int64
	nextIdx       []int
	matchIdx      []int
	isSnapshoting bool

	leaderId         int64
	electionTimer    *time.Timer
	heartBeatTimer   *time.Timer
	heartBeatTimeout uint64
	baseElecTimeout  uint64
}

func MakeRaft(peers []*RaftClientEnd, me int, applych chan *pb.ApplyMsg, hearttime uint64, electiontime uint64) *Raft {
	newraft := &Raft{
		peers:            peers,
		me:               me,
		dead:             0,
		applyCh:          applych,
		replicatorCond:   make([]*sync.Cond, len(peers)),
		role:             FOLLOWER,
		curTerm:          0,
		votedFor:         None,
		grantedVotes:     0,
		isSnapshoting:    false,
		logs:             MakeMemRaftLog(),
		commitIdx:        0,
		lastApplied:      0,
		nextIdx:          make([]int, len(peers)),
		matchIdx:         make([]int, len(peers)),
		heartBeatTimer:   time.NewTimer(time.Millisecond * time.Duration(hearttime)),
		electionTimer:    time.NewTimer(time.Millisecond * time.Duration(MakeAnRandomElectionTimeout(int(electiontime)))),
		baseElecTimeout:  electiontime,
		heartBeatTimeout: hearttime,
	}
	newraft.applyCond = sync.NewCond(&newraft.mu)
	lastLog := newraft.logs.GetMemLast()
	for _, peer := range peers {
		fmt.Printf("peer addr:%s   id:%d ", peer.addr, peer.id)
		newraft.matchIdx[peer.id], newraft.nextIdx[peer.id] = 0, int(lastLog.Index+1)
		if int(peer.id) != me {
			newraft.replicatorCond[peer.id] = sync.NewCond(&sync.Mutex{})
			go newraft.Replicator(peer)
		}
	}

	go newraft.Ticker()

	go newraft.Applier()

	return newraft
}

// Handle heartbeat timeouts and election timeouts
func (raft *Raft) Ticker() {
	for !raft.IsKilled() {
		select {
		case <-raft.electionTimer.C:
			{
				raft.mu.Lock()
				raft.ChangeRole(CANDIDATE)
				raft.curTerm += 1
				raft.StartNewElection()
				raft.electionTimer.Reset(time.Millisecond * time.Duration(MakeAnRandomElectionTimeout(int(raft.baseElecTimeout))))
				raft.mu.Unlock()
			}
		case <-raft.heartBeatTimer.C:
			{
				if raft.role == LEADER {
					raft.BroadcastHeartbeat()
					raft.heartBeatTimer.Reset(time.Millisecond * time.Duration(raft.heartBeatTimeout))
				}
			}
		}
	}
}

// Applier() Write the commited message to the applyCh channel
// and update lastApplied
func (raft *Raft) Applier() {
	for !raft.IsKilled() {
		raft.mu.Lock()
		for raft.lastApplied >= raft.commitIdx {
			log.MainLogger.Debug().Msgf("applier ... ")
			raft.applyCond.Wait()
		}
		firstIndex, commitIndex, lastApplied := raft.logs.GetMemFirst().Index, raft.commitIdx, raft.lastApplied
		entries := make([]*pb.Entry, commitIndex-lastApplied)
		copy(entries, raft.logs.GetMemRange(lastApplied+1-int64(firstIndex), commitIndex+1-int64(firstIndex)))
		log.MainLogger.Debug().Msgf("%d, applies entries %d-%d in term %d", raft.me, raft.lastApplied, commitIndex, raft.curTerm)
		raft.mu.Unlock()
		for _, entry := range entries {
			raft.applyCh <- &pb.ApplyMsg{
				CommandValid: true,
				Command:      entry.Data,
				CommandTerm:  int64(entry.Term),
				CommandIndex: entry.Index,
			}
		}
		raft.mu.Lock()
		raft.lastApplied = int64(Max(int(raft.lastApplied), int(commitIndex)))
		raft.mu.Unlock()
	}
}

// Replicator manager duplicate run
func (raft *Raft) Replicator(peer *RaftClientEnd) {
	raft.replicatorCond[peer.id].L.Lock()
	defer raft.replicatorCond[peer.id].L.Unlock()
	for !raft.IsKilled() {
		PrintDebugLog(fmt.Sprintf("peer id:%d wait for replicating...", peer.id))
		for !(raft.role == LEADER && raft.matchIdx[peer.id] < int(raft.logs.GetMemLast().Index)) {
			raft.replicatorCond[peer.id].Wait()
		}
		raft.replicatorOneRound(peer)
	}
}

// replicateOneRound Leader replicates log entries to followers
func (raft *Raft) replicatorOneRound(peer *RaftClientEnd) {
	raft.mu.RLock()
	if raft.role != LEADER {
		raft.mu.RUnlock()
		return
	}
	prevLogIndex := uint64(raft.nextIdx[peer.id] - 1)
	PrintDebugLog(fmt.Sprintf("leader prevLogIndex %d", prevLogIndex))
	// snapshot
	if prevLogIndex < uint64(raft.logs.GetMemFirst().GetIndex()) {
		firstLog := raft.logs.GetMemFirst()
		snapShotReq := &pb.InstallSnapshotRequest{
			Term:              raft.curTerm,
			LeaderId:          int64(raft.me),
			LastIncludedIndex: firstLog.Index,
			LastIncludedTerm:  int64(firstLog.Term),
			Data:              raft.ReadSnapshot(),
		}
		raft.mu.RUnlock()
		log.MainLogger.Debug().Msgf("send snapshot to %s with %s\n", peer.addr, snapShotReq.String())
		snapShotResp, err := (*peer.raftServiceCli).Snapshot(context.Background(), snapShotReq)
		if err != nil {
			log.MainLogger.Debug().Msgf("send snapshot to %s failed %v\n", peer.addr, err.Error())
		}

		raft.mu.Lock()
		log.MainLogger.Debug().Msgf("send snapshot to %s with resp %s\n", peer.addr, snapShotResp.String())

		if snapShotResp != nil {
			if raft.role == LEADER && raft.curTerm == snapShotReq.Term {
				if snapShotResp.Term < raft.curTerm {
					raft.ChangeRole(FOLLOWER)
					raft.curTerm = snapShotReq.Term
					raft.votedFor = -1
					// TO DO PERSIST
				} else {
					log.MainLogger.Debug().Msgf("set peer %d matchIdx %d\n", peer.id, snapShotReq.LastIncludedIndex)
					// raft.matchIdx[peer.id] = snapShotResp.Lad
				}
			}
		}
	} else {
		firstIndex := raft.logs.GetMemFirst().Index
		log.MainLogger.Debug().Msgf("first log index %d", firstIndex)
		entries := make([]*pb.Entry, len(raft.logs.GetMemAfter(int64(prevLogIndex)-firstIndex+1)))
		copy(entries, raft.logs.GetMemAfter(int64(prevLogIndex)+1-firstIndex))
		appendEntReq := &pb.AppendEntriesRequest{
			Term:         raft.curTerm,
			LeaderId:     int64(raft.me),
			PrevLogIndex: int64(prevLogIndex),
			PrevLogTerm:  int64(raft.logs.GetMemEntry(int64(prevLogIndex) - firstIndex).Term),
			Entries:      entries,
			LeaderCommit: raft.commitIdx,
		}
		raft.mu.Unlock()

		resp, err := (*peer.raftServiceCli).AppendEntries(context.Background(), appendEntReq)
		if err != nil {
			log.MainLogger.Debug().Msgf("send append entries to %s failed %v\n", peer.addr, err.Error())
		}
		if raft.role == LEADER {
			if resp != nil {
				if resp.Success {
					log.MainLogger.Debug().Msgf("send heart beat to %s success", peer.addr)
					raft.matchIdx[peer.id] = int(appendEntReq.PrevLogIndex) + len(appendEntReq.Entries)
					raft.nextIdx[peer.id] = raft.matchIdx[peer.id] + 1
					raft.advanceCommitIndexForLeader()
				} else {
					if resp.Term > raft.curTerm {
						raft.ChangeRole(FOLLOWER)
						raft.curTerm = resp.Term
						raft.votedFor = None
						// TO DO persist
					} else if resp.Term == raft.curTerm {
						raft.nextIdx[peer.id] = int(resp.ConflictIndex)
						if resp.ConflictTerm != -1 {
							for i := appendEntReq.PrevLogIndex; i >= firstIndex; i-- {
								if raft.logs.GetMemEntry(i-int64(firstIndex)).Term == uint64(resp.GetConflictTerm()) {
									raft.nextIdx[peer.id] = int(i + 1)
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

// HandleRequestVote  handle append entries from other node
func (raft *Raft) HandleAppendEntries(req *pb.AppendEntriesRequest, resp *pb.AppendEntriesResponse) {
	raft.mu.Lock()
	defer raft.mu.Unlock()
	//TO DO PERSIST

	if req.Term < raft.curTerm {
		resp.Term = raft.curTerm
		resp.Success = false
		return
	}

	if req.Term > raft.curTerm {
		raft.curTerm = req.Term
		raft.votedFor = None
	}

	raft.ChangeRole(FOLLOWER)
	raft.leaderId = req.LeaderId
	raft.electionTimer.Reset(time.Millisecond * time.Duration(MakeAnRandomElectionTimeout(int(raft.baseElecTimeout))))

	if req.PrevLogIndex < raft.logs.GetMemFirst().Index {
		resp.Term = 0
		resp.Success = false
		log.MainLogger.Debug().Msgf("peer %d reject append entires request from %d", raft.me, req.LeaderId)
		return
	}

	if !raft.MatchLog(req.PrevLogTerm, req.PrevLogIndex) {
		resp.Term, resp.Success = raft.curTerm, false
		lastIndex := raft.logs.GetMemLast().Index
		if lastIndex < req.PrevLogIndex {
			log.MainLogger.Debug().Msgf("log confict with term %d, index %d", -1, lastIndex+1)
			resp.ConflictIndex, resp.ConflictTerm = lastIndex+1, -1
		} else {
			firstIndex := raft.logs.GetMemFirst().Index
			resp.ConflictTerm = int64(raft.logs.GetMemEntry(req.PrevLogIndex - firstIndex).Term)
			index := req.PrevLogIndex - 1
			for index >= firstIndex && raft.logs.GetMemEntry(index-firstIndex).Term == uint64(resp.ConflictIndex) {
				index--
			}
			resp.ConflictIndex = index
		}
		return
	}

	firstIndex := raft.logs.GetMemFirst().Index
	for index, entry := range req.Entries {
		if int(entry.Index-firstIndex) > raft.logs.MemLogItemCount() || raft.logs.GetMemEntry(entry.Index-firstIndex).Term != entry.Term {
			raft.logs.EraseMemAfter(entry.Index - firstIndex)
			for _, newEnt := range req.Entries[index:] {
				raft.logs.MemAppend(newEnt)
			}
			break
		}
	}

	raft.advanceCommitIndexForFollower(int(req.LeaderCommit))
	resp.Term, resp.Success = raft.curTerm, true
}

// HandleRequestVote  handle request vote from other node
func (raft *Raft) HandleRequestVote(req *pb.RequestVoteRequest, resp *pb.RequestVoteResponse) {
	raft.mu.Lock()
	defer raft.mu.Unlock()
	// TO DO persistraft

	log.MainLogger.Debug().Msgf("Handle vote request: %s", req.String())

	canVote := raft.votedFor == req.CandidateId ||
		(raft.votedFor == None && raft.leaderId == None) ||
		req.Term > raft.curTerm

	if canVote && raft.isUpToDate(req.LastLogIndex, req.LastLogTerm) {
		resp.Term, resp.VoteGranted = raft.curTerm, true
	} else {
		resp.Term, resp.VoteGranted = raft.curTerm, false
		return
	}

	raft.votedFor = req.CandidateId
	raft.electionTimer.Reset(time.Millisecond * time.Duration(MakeAnRandomElectionTimeout(int(raft.baseElecTimeout))))
}

// Append append a new command to it's logs
func (raft *Raft) Append(command []byte) *pb.Entry {
	lastLog := raft.logs.GetMemLast()
	newLog := &pb.Entry{
		Index: lastLog.Index + 1,
		Term:  uint64(raft.curTerm),
		Data:  command,
	}
	raft.logs.MemAppend(newLog)
	raft.matchIdx[raft.me] = int(newLog.Index)
	raft.nextIdx[raft.me] = raft.matchIdx[raft.me] + 1
	// TO DO persist
	return newLog
}

// Propose the interface to the appplication propose a operation
func (raft *Raft) Propose(payload []byte) (int, int, bool) {
	raft.mu.Lock()
	defer raft.mu.Unlock()
	if raft.role != LEADER {
		return -1, -1, false
	}
	if raft.isSnapshoting {
		return -1, -1, false
	}
	newLog := raft.Append(payload)
	raft.BroadcastAppend()
	return int(newLog.Index), int(newLog.Term), true
}

// Election  make a new election
//
func (raft *Raft) StartNewElection() {
	log.MainLogger.Debug().Msgf("%d start a new election \n", raft.me)
	raft.mu.Lock()
	defer raft.mu.Unlock()
	raft.grantedVotes = 1
	raft.votedFor = int64(raft.me)
	voteReq := &pb.RequestVoteRequest{
		Term:         raft.curTerm,
		CandidateId:  int64(raft.me),
		LastLogIndex: raft.logs.GetMemLast().Index,
		LastLogTerm:  int64(raft.logs.GetMemLast().Term),
	}
	// TO DO PERSIST RAFT STATE
	for _, peer := range raft.peers {
		if peer.id == uint64(raft.me) || raft.role == LEADER {
			continue
		}
		go func(peer *RaftClientEnd) {
			log.MainLogger.Debug().Msgf("send request vote to %s %s\n", peer.addr, voteReq.String())

			requestVoteResp, err := (*peer.raftServiceCli).RequestVote(context.Background(), voteReq)

			if err != nil {
				log.MainLogger.Debug().Msgf("send request vote to %s failed %v \n", peer.addr, err.Error())
			}

			if requestVoteResp != nil {
				raft.mu.Lock()
				defer raft.mu.Unlock()
				log.MainLogger.Debug().Msgf("send request vote to %s recive -> %s, curterm %d, req term %d", peer.addr, requestVoteResp.String(), raft.curTerm, voteReq.Term)
				if raft.curTerm == voteReq.Term && raft.role == CANDIDATE {
					if requestVoteResp.VoteGranted {
						log.MainLogger.Debug().Msgf("I got a vote \n")
						raft.IncrGrantedVotes()
						if raft.grantedVotes > len(raft.peers)/2 {
							log.MainLogger.Debug().Msgf("node %d get majority votes int term %d ", raft.me, raft.curTerm)
							raft.ChangeRole(LEADER)
							raft.BroadcastHeartbeat()
							raft.grantedVotes = 0
						}
					} else if requestVoteResp.Term > raft.curTerm {
						raft.ChangeRole(FOLLOWER)
						raft.curTerm, raft.votedFor = requestVoteResp.Term, None
						// TO DO PERSISTRAFTESTATE
					}
				}
			}
		}(peer)
	}
}

// install snapshot from leader
func (raft *Raft) HandleInstallSnapshot(request *pb.InstallSnapshotRequest, response *pb.InstallSnapshotResponse) {

}

func (raft *Raft) advanceCommitIndexForLeader() {
	sort.Ints(raft.matchIdx)
	n := len(raft.matchIdx)
	newCommitIndex := raft.matchIdx[n/2]
	if int64(newCommitIndex) > raft.commitIdx {
		if raft.MatchLog(raft.curTerm, int64(newCommitIndex)) {
			log.MainLogger.Debug().Msgf("peer %d advance commit index %d at term %d", raft.me, raft.commitIdx, raft.curTerm)
			raft.commitIdx = int64(newCommitIndex)
			raft.applyCond.Signal()
		}
	}
}

func (raft *Raft) advanceCommitIndexForFollower(leaderCommit int) {
	newCommitIndex := Min(leaderCommit, int(raft.logs.GetMemLast().Index))
	if newCommitIndex > int(raft.commitIdx) {
		PrintDebugLog(fmt.Sprintf("peer %d advance commit index %d at term %d", raft.me, raft.commitIdx, raft.curTerm))
		raft.commitIdx = int64(newCommitIndex)
		raft.applyCond.Signal()
	}
}

// BroadcastAppend broadcast append to peers
func (raft *Raft) BroadcastAppend() {
	for _, peer := range raft.peers {
		if peer.id == uint64(raft.me) {
			continue
		}
		raft.replicatorCond[peer.id].Signal()
	}
}

// BroadcastHeartbeat broadcast heartbeat to peers
func (raft *Raft) BroadcastHeartbeat() {
	for _, peer := range raft.peers {
		if int(peer.id) == raft.me {
			continue
		}
		log.MainLogger.Debug().Msgf("send heart beat to %s", peer.addr)
		go func(peer *RaftClientEnd) {
			raft.replicatorOneRound(peer)
		}(peer)
	}
}

func (raft *Raft) IsKilled() bool {
	return atomic.LoadInt32(&raft.dead) == 1
}

func (raft *Raft) GetFirstLogEnt() *pb.Entry {
	return raft.logs.GetMemFirst()
}

func (raft *Raft) IncrCurrentTerm() {
	// raft.mu.Lock()
	// defer raft.mu.Unlock()
	atomic.AddInt64(&raft.curTerm, 1)
	// raft.curTerm += 1
}

func (raft *Raft) GetState() (int, bool) {
	raft.mu.RLock()
	defer raft.mu.RUnlock()
	return int(raft.curTerm), raft.role == LEADER
}

func (raft *Raft) IncrGrantedVotes() {
	raft.mu.Lock()
	defer raft.mu.Unlock()
	raft.grantedVotes += 1
	// atomic.AddInt32(&raft.grantedVotes, 1)
}

func (raft *Raft) ReInitLog() {
	// raft.logs.
}

func (raft *Raft) GetLeaderId() int64 {
	raft.mu.RLock()
	defer raft.mu.RUnlock()
	return raft.leaderId
}

func (raft *Raft) GetLogCount() int {
	raft.mu.Lock()
	defer raft.mu.Unlock()
	return raft.logs.MemLogItemCount()
}

// MatchLog is log matched
//
func (raft *Raft) MatchLog(term, index int64) bool {
	return index <= int64(raft.logs.GetMemLast().Index) &&
		raft.logs.GetMemEntry(index-int64(raft.logs.GetMemFirst().Index)).Term == uint64(term)
}

// change raft node's role to new role
func (raft *Raft) ChangeRole(newrole RAFTROLE) {
	if raft.role == newrole {
		return
	}
	raft.role = newrole
	fmt.Printf("node's role change to -> %s\n", RoleToString(newrole))
	switch newrole {
	case FOLLOWER:
		raft.heartBeatTimer.Stop()
		raft.electionTimer.Reset(time.Duration(MakeAnRandomElectionTimeout(int(raft.baseElecTimeout))) * time.Millisecond)
	case CANDIDATE:

	case LEADER:
		lastLog := raft.logs.GetMemLast()
		raft.leaderId = int64(raft.me)
		for i := 0; i < len(raft.peers); i++ {
			raft.matchIdx[i], raft.nextIdx[i] = 0, int(lastLog.Index+1)
		}
		raft.electionTimer.Stop()
		raft.heartBeatTimer.Reset(time.Duration(raft.heartBeatTimeout) * time.Millisecond)
	}
}

func (raft *Raft) CondInstallSnapshot(lastIncluedTerm int, lastIncludedIndex int, snapshot []byte) bool {
	return true
}

func (raft *Raft) isUpToDate(lastIdx, term int64) bool {
	return term > int64(raft.logs.GetMemLast().Term) || (term == int64(raft.logs.GetMemLast().Term) && lastIdx >= int64(raft.logs.GetMemLast().Index))
}

// take a snapshot
func (raft *Raft) Snapshot(index int, snapshot []byte) {
	// raft.mu.Lock()
	// defer raft.mu.Unlock()
	// raft.isSnapshoting = true
	// snapshotIndex := raft.logs.GetMemFirst().Index
}

func (raft *Raft) ReadSnapshot() []byte {
	return nil
}

// CloseEndsConn close rpc client connect
func (raft *Raft) CloseEndsConn() {
	for _, peer := range raft.peers {
		peer.CloseAllConn()
	}
}
