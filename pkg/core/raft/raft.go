// Copyright [2022] [WellWood]

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
	"fmt"
	"sync"
	"sync/atomic"
	"time"

	pb "github.com/eraft-io/eraft/pkg/protocol"
)

type RAFTROLE uint8

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
		votedFor:         -1,
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

// change raft node's role to new role
func (raft *Raft) ChangeRole(newrole RAFTROLE) {
	if raft.role == newrole {
		return
	}
	raft.role = newrole
	fmt.Printf("node's role change to -> %s\n", RoleToString(newrole))
}

func (raft *Raft) Applier() {

}

//
// Replicator manager duplicate run
//

func (raft *Raft) Replicator(peer *RaftClientEnd) {
	raft.replicatorCond[peer.id].L.Lock()
	defer raft.replicatorCond[peer.id].L.Unlock()
	for !raft.IsKilled() {
		PrintDebugLog(fmt.Sprintf("peer id:%d wait for replicating...", peer.id))
		for !(raft.role == LEADER && raft.matchIdx[peer.id] < int(raft.logs.GetMemLast().Index)) {
			raft.replicatorCond[peer.id].Wait()
		}
		raft.ReplicatorOneRound(peer)
	}
}

// replicateOneRound Leader replicates log entries to followers
func (raft *Raft) ReplicatorOneRound(peer *RaftClientEnd) {
	raft.mu.RLock()
	if raft.role != LEADER {
		raft.mu.RUnlock()
		return
	}
	prevLogIndex := uint64(raft.nextIdx[peer.id] - 1)
	PrintDebugLog(fmt.Sprintf("leader prevLogIndex %d", prevLogIndex))
	// snapshot
	if prevLogIndex < uint64(raft.logs.GetMemFirst().GetIndex()) {
		// firstLog := raft.logs.GetMemFirst()
		// snapShotReq := &pb.InstallSnapshotRequest{
		// 	Term: raft.curTerm,
		// 	LeaderId: int64(raft.me),
		// 	LastIncludedIndex: firstLog.Index,
		// 	LastIncludedTerm: int64(firstLog.Term),
		// 	Data: raft.ReadSnapshot(),
		// }
	} else {
		firstIndex := raft.logs.GetMemFirst().Index
		PrintDebugLog(fmt.Sprintf("first log index %d", firstIndex))
		entries := make([]*pb.Entry, len(raft.logs.EraseMemBefore(int64(prevLogIndex)+1-firstIndex)))
		copy(entries, raft.logs.EraseMemBefore(int64(prevLogIndex)+1-firstIndex))

	}
}

func (rf *Raft) IsKilled() bool {
	return atomic.LoadInt32(&rf.dead) == 1
}
