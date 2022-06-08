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
	"fmt"
	"sort"
	"sync"
	"sync/atomic"
	"time"

	pb "github.com/eraft-io/eraft/raftpb"
	"github.com/eraft-io/eraft/storage_eng"
)

type NodeRole uint8

//
// raft node state
//
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

//
// raft stack definition
//
type Raft struct {
	mu             sync.RWMutex
	peers          []*RaftClientEnd // rpc client end
	me_            int
	dead           int32
	applyCh        chan *pb.ApplyMsg
	applyCond      *sync.Cond
	replicatorCond []*sync.Cond
	role           NodeRole
	curTerm        int64
	votedFor       int64
	grantedVotes   int
	logs           *RaftLog
	persister      *RaftLog
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

func MakeRaft(peers []*RaftClientEnd, me int, newdbEng storage_eng.KvStore, applyCh chan *pb.ApplyMsg, heartbeatTimeOutMs uint64, baseElectionTimeOutMs uint64) *Raft {
	rf := &Raft{
		peers:            peers,
		me_:              me,
		dead:             0,
		applyCh:          applyCh,
		replicatorCond:   make([]*sync.Cond, len(peers)),
		role:             NodeRoleFollower,
		curTerm:          0,
		votedFor:         VOTE_FOR_NO_ONE,
		grantedVotes:     0,
		isSnapshoting:    false,
		logs:             MakePersistRaftLog(newdbEng),
		persister:        MakePersistRaftLog(newdbEng),
		nextIdx:          make([]int, len(peers)),
		matchIdx:         make([]int, len(peers)),
		heartbeatTimer:   time.NewTimer(time.Millisecond * time.Duration(heartbeatTimeOutMs)),
		electionTimer:    time.NewTimer(time.Millisecond * time.Duration(MakeAnRandomElectionTimeout(int(baseElectionTimeOutMs)))),
		baseElecTimeout:  baseElectionTimeOutMs,
		heartBeatTimeout: heartbeatTimeOutMs,
	}
	rf.curTerm, rf.votedFor = rf.persister.ReadRaftState()
	rf.ReInitLog()
	rf.applyCond = sync.NewCond(&rf.mu)
	lastLog := rf.logs.GetLast()
	for _, peer := range peers {
		fmt.Printf("peer addr %s id %d", peer.addr, peer.id)
		rf.matchIdx[peer.id], rf.nextIdx[peer.id] = 0, int(lastLog.Index+1)
		if int(peer.id) != me {
			rf.replicatorCond[peer.id] = sync.NewCond(&sync.Mutex{})
			go rf.Replicator(peer)
		}
	}

	go rf.Tick()

	go rf.Applier()

	return rf
}

func (rf *Raft) PersistRaftState() {
	rf.persister.PersistRaftState(rf.curTerm, rf.votedFor)
}

func (rf *Raft) Kill() {
	atomic.StoreInt32(&rf.dead, 1)
}

func (rf *Raft) IsKilled() bool {
	return atomic.LoadInt32(&rf.dead) == 1
}

func (rf *Raft) GetFirstLogEnt() *pb.Entry {
	return rf.logs.GetFirst()
}

func (rf *Raft) SwitchRaftNodeRole(role NodeRole) {
	if rf.role == role {
		return
	}
	rf.role = role
	fmt.Printf("note change state to -> %s \n", NodeToString(role))
	switch role {
	case NodeRoleFollower:
		rf.heartbeatTimer.Stop()
		rf.electionTimer.Reset(time.Duration(MakeAnRandomElectionTimeout(int(rf.baseElecTimeout))) * time.Millisecond)
	case NodeRoleCandidate:
	case NodeRoleLeader:
		// become leader，set replica (matchIdx and nextIdx) processs table
		lastLog := rf.logs.GetLast()
		rf.leaderId = int64(rf.me_)
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

//
// HandleRequestVote  handle request vote from other node
//
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

	lastLog := rf.logs.GetLast()

	if !(req.LastLogTerm > int64(lastLog.Term) || (req.LastLogTerm == int64(lastLog.Term) && req.LastLogIndex >= lastLog.Index)) {
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
		PrintDebugLog(fmt.Sprintf("peer %d reject append entires request from %d", rf.me_, req.LeaderId))
		return
	}

	if !rf.MatchLog(req.PrevLogTerm, req.PrevLogIndex) {
		resp.Term = rf.curTerm
		resp.Success = false
		lastIndex := rf.logs.GetLast().Index
		if lastIndex < req.PrevLogIndex {
			PrintDebugLog(fmt.Sprintf("log confict with term %d, index %d", -1, lastIndex+1))
			resp.ConflictTerm = -1
			resp.ConflictIndex = lastIndex + 1
		} else {
			firstIndex := rf.logs.GetFirst().Index
			resp.ConflictTerm = int64(rf.logs.GetEntry(req.PrevLogIndex - int64(firstIndex)).Term)
			index := req.PrevLogIndex - 1
			for index >= int64(firstIndex) && rf.logs.GetEntry(index-firstIndex).Term == uint64(resp.ConflictTerm) {
				index--
			}
			resp.ConflictIndex = index
		}
		return
	}

	firstIndex := rf.logs.GetFirst().Index
	for index, entry := range req.Entries {
		if int(entry.Index-firstIndex) >= rf.logs.LogItemCount() || rf.logs.GetEntry(entry.Index-firstIndex).Term != entry.Term {
			rf.logs.EraseAfter(entry.Index-firstIndex, true)
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
		PrintDebugLog("lastIncludedIndex > last log id")
		rf.logs.ReInitLogs()
	} else {
		PrintDebugLog("install snapshot del old log")
		rf.logs.EraseBeforeWithDel(int64(lastIncludedIndex) - rf.logs.GetFirst().Index)
		rf.logs.SetEntFirstData([]byte{})
	}
	// update dummy entry with lastIncludedTerm and lastIncludedIndex
	rf.logs.SetEntFirstTermAndIndex(int64(lastIncluedTerm), int64(lastIncludedIndex))

	rf.lastApplied = int64(lastIncludedIndex)
	rf.commitIdx = int64(lastIncludedIndex)

	// rf.logs.PersisSnapshot(snapshot)
	return true
}

//
// take a snapshot
//
func (rf *Raft) Snapshot(index int, snapshot []byte) {
	rf.mu.Lock()
	defer rf.mu.Unlock()
	rf.isSnapshoting = true
	snapshotIndex := rf.logs.GetFirstLogId()
	if index <= int(snapshotIndex) {
		rf.isSnapshoting = false
		PrintDebugLog("reject snapshot, current snapshotIndex is larger in cur term")
		return
	}
	rf.logs.EraseBeforeWithDel(int64(index) - int64(snapshotIndex))
	rf.logs.SetEntFirstData([]byte{}) // 第一个操作日志号设为空
	PrintDebugLog(fmt.Sprintf("del log entry before idx %d", index))
	rf.isSnapshoting = false
	rf.logs.PersisSnapshot(snapshot)
}

func (rf *Raft) ReadSnapshot() []byte {
	b, err := rf.logs.ReadSnapshot()
	if err != nil {
		// panic(err)
		fmt.Println(err.Error())
	}
	return b
}

//
// install snapshot from leader
//
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
	newCommitIndex := rf.matchIdx[n-(n/2+1)]
	if newCommitIndex > int(rf.commitIdx) {
		if rf.MatchLog(rf.curTerm, int64(newCommitIndex)) {
			PrintDebugLog(fmt.Sprintf("peer %d advance commit index %d at term %d", rf.me_, rf.commitIdx, rf.curTerm))
			rf.commitIdx = int64(newCommitIndex)
			rf.applyCond.Signal()
		}
	}
}

func (rf *Raft) advanceCommitIndexForFollower(leaderCommit int) {
	newCommitIndex := Min(leaderCommit, int(rf.logs.GetLast().Index))
	if newCommitIndex > int(rf.commitIdx) {
		PrintDebugLog(fmt.Sprintf("peer %d advance commit index %d at term %d", rf.me_, rf.commitIdx, rf.curTerm))
		rf.commitIdx = int64(newCommitIndex)
		rf.applyCond.Signal()
	}
}

//
// MatchLog is log matched
//
func (rf *Raft) MatchLog(term, index int64) bool {
	return index <= int64(rf.logs.GetLast().Index) && rf.logs.GetEntry(index-int64(rf.logs.GetFirst().Index)).Term == uint64(term)
}

//
// Election  make a new election
//
func (rf *Raft) Election() {
	fmt.Printf("%d start election \n", rf.me_)
	rf.IncrGrantedVotes()
	rf.votedFor = int64(rf.me_)
	voteReq := &pb.RequestVoteRequest{
		Term:         rf.curTerm,
		CandidateId:  int64(rf.me_),
		LastLogIndex: int64(rf.logs.GetLast().Index),
		LastLogTerm:  int64(rf.logs.GetLast().Term),
	}
	rf.PersistRaftState()
	for _, peer := range rf.peers {
		if int(peer.id) == rf.me_ {
			continue
		}
		go func(peer *RaftClientEnd) {
			PrintDebugLog(fmt.Sprintf("send request vote to %s %s\n", peer.addr, voteReq.String()))

			requestVoteResp, err := (*peer.raftServiceCli).RequestVote(context.Background(), voteReq)
			if err != nil {
				PrintDebugLog(fmt.Sprintf("send request vote to %s failed %v\n", peer.addr, err.Error()))
			}
			if requestVoteResp != nil {
				rf.mu.Lock()
				defer rf.mu.Unlock()
				PrintDebugLog(fmt.Sprintf("send request vote to %s recive -> %s, curterm %d, req term %d", peer.addr, requestVoteResp.String(), rf.curTerm, voteReq.Term))
				if rf.curTerm == voteReq.Term && rf.role == NodeRoleCandidate {
					if requestVoteResp.VoteGranted {
						// success granted the votes
						PrintDebugLog("I grant vote")
						rf.IncrGrantedVotes()
						if rf.grantedVotes > len(rf.peers)/2 {
							PrintDebugLog(fmt.Sprintf("node %d get majority votes int term %d ", rf.me_, rf.curTerm))
							rf.SwitchRaftNodeRole(NodeRoleLeader)
							rf.BroadcastHeartbeat()
							rf.grantedVotes = 0
						}
					} else if requestVoteResp.Term > rf.curTerm {
						// request vote reject
						rf.SwitchRaftNodeRole(NodeRoleFollower)
						rf.curTerm, rf.votedFor = requestVoteResp.Term, -1
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
		if peer.id == uint64(rf.me_) {
			continue
		}
		rf.replicatorCond[peer.id].Signal()
	}
}

//
// BroadcastHeartbeat broadcast heartbeat to peers
//
func (rf *Raft) BroadcastHeartbeat() {
	for _, peer := range rf.peers {
		if int(peer.id) == rf.me_ {
			continue
		}
		PrintDebugLog(fmt.Sprintf("send heart beat to %s", peer.addr))
		go func(peer *RaftClientEnd) {
			rf.replicateOneRound(peer)
		}(peer)
	}
}

//
// Tick raft heart, this ticket trigger raft main flow running
//
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
	rf.matchIdx[rf.me_] = int(newLog.Index)
	rf.nextIdx[rf.me_] = int(newLog.Index) + 1
	rf.PersistRaftState()
	return newLog
}

//
// CloseEndsConn close rpc client connect
//
func (rf *Raft) CloseEndsConn() {
	for _, peer := range rf.peers {
		peer.CloseAllConn()
	}
}

//
// Replicator manager duplicate run
//
func (rf *Raft) Replicator(peer *RaftClientEnd) {
	rf.replicatorCond[peer.id].L.Lock()
	defer rf.replicatorCond[peer.id].L.Unlock()
	for !rf.IsKilled() {
		PrintDebugLog("peer id wait for replicating...")
		for !(rf.role == NodeRoleLeader && rf.matchIdx[peer.id] < int(rf.logs.GetLast().Index)) {
			rf.replicatorCond[peer.id].Wait()
		}
		rf.replicateOneRound(peer)
	}
}

//
// replicateOneRound duplicate log entries to other nodes in the cluster
//
func (rf *Raft) replicateOneRound(peer *RaftClientEnd) {
	rf.mu.RLock()
	if rf.role != NodeRoleLeader {
		rf.mu.RUnlock()
		return
	}
	prevLogIndex := uint64(rf.nextIdx[peer.id] - 1)
	PrintDebugLog(fmt.Sprintf("leader prevLogIndex %d", prevLogIndex))
	if prevLogIndex < uint64(rf.logs.GetFirst().Index) {
		firstLog := rf.logs.GetFirst()
		snapShotReq := &pb.InstallSnapshotRequest{
			Term:              rf.curTerm,
			LeaderId:          int64(rf.me_),
			LastIncludedIndex: firstLog.Index,
			LastIncludedTerm:  int64(firstLog.Term),
			Data:              rf.ReadSnapshot(),
		}

		rf.mu.RUnlock()

		PrintDebugLog(fmt.Sprintf("send snapshot to %s with %s\n", peer.addr, snapShotReq.String()))

		snapShotResp, err := (*peer.raftServiceCli).Snapshot(context.Background(), snapShotReq)
		if err != nil {
			PrintDebugLog(fmt.Sprintf("send snapshot to %s failed %v\n", peer.addr, err.Error()))
		}

		rf.mu.Lock()
		PrintDebugLog(fmt.Sprintf("send snapshot to %s with resp %s\n", peer.addr, snapShotResp.String()))

		if snapShotResp != nil {
			if rf.role == NodeRoleLeader && rf.curTerm == snapShotReq.Term {
				if snapShotResp.Term > rf.curTerm {
					rf.SwitchRaftNodeRole(NodeRoleFollower)
					rf.curTerm = snapShotResp.Term
					rf.votedFor = -1
					rf.PersistRaftState()
				} else {
					PrintDebugLog(fmt.Sprintf("set peer %d matchIdx %d\n", peer.id, snapShotReq.LastIncludedIndex))
					rf.matchIdx[peer.id] = int(snapShotReq.LastIncludedIndex)
					rf.nextIdx[peer.id] = int(snapShotReq.LastIncludedIndex) + 1
				}
			}
		}
		rf.mu.Unlock()
	} else {
		firstIndex := rf.logs.GetFirst().Index
		PrintDebugLog(fmt.Sprintf("first log index %d", firstIndex))
		entries := make([]*pb.Entry, len(rf.logs.EraseBefore(int64(prevLogIndex)+1-firstIndex)))
		copy(entries, rf.logs.EraseBefore(int64(prevLogIndex)+1-firstIndex))
		appendEntReq := &pb.AppendEntriesRequest{
			Term:         rf.curTerm,
			LeaderId:     int64(rf.me_),
			PrevLogIndex: int64(prevLogIndex),
			PrevLogTerm:  int64(rf.logs.GetEntry(int64(prevLogIndex) - firstIndex).Term),
			Entries:      entries,
			LeaderCommit: rf.commitIdx,
		}
		rf.mu.RUnlock()

		// send empty ae to peers
		resp, err := (*peer.raftServiceCli).AppendEntries(context.Background(), appendEntReq)
		if err != nil {
			PrintDebugLog(fmt.Sprintf("send append entries to %s failed %v\n", peer.addr, err.Error()))
		}
		if rf.role == NodeRoleLeader && rf.curTerm == appendEntReq.Term {
			if resp != nil {
				// deal with appendRnt resp
				if resp.Success {
					PrintDebugLog(fmt.Sprintf("send heart beat to %s success", peer.addr))
					rf.matchIdx[peer.id] = int(appendEntReq.PrevLogIndex) + len(appendEntReq.Entries)
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
							for i := appendEntReq.PrevLogIndex; i >= int64(firstIndex); i-- {
								if rf.logs.GetEntry(i-int64(firstIndex)).Term == uint64(resp.ConflictTerm) {
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

//
// Applier() Write the commited message to the applyCh channel
// and update lastApplied
//
func (rf *Raft) Applier() {
	for !rf.IsKilled() {
		rf.mu.Lock()
		for rf.lastApplied >= rf.commitIdx {
			PrintDebugLog("applier ...")
			rf.applyCond.Wait()
		}

		firstIndex, commitIndex, lastApplied := rf.logs.GetFirst().Index, rf.commitIdx, rf.lastApplied
		entries := make([]*pb.Entry, commitIndex-lastApplied)
		copy(entries, rf.logs.GetRange(lastApplied+1-int64(firstIndex), commitIndex+1-int64(firstIndex)))
		PrintDebugLog(fmt.Sprintf("%d, applies entries %d-%d in term %d", rf.me_, rf.lastApplied, commitIndex, rf.curTerm))

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
		rf.lastApplied = int64(Max(int(rf.lastApplied), int(commitIndex)))
		rf.mu.Unlock()
	}
}
