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
	"sync"

	"github.com/eraft-io/eraft/pkg/engine"
	pb "github.com/eraft-io/eraft/pkg/protocol"
)

type RaftLog struct {
	mu       sync.RWMutex
	firstIdx uint64
	lastIdx  uint64
	items    []*pb.Entry
	dbEng    engine.KvStore
}

type LogOp interface {
	GetFirst() *pb.Entry

	LogItemCount() int

	EraseBefore(idx int64) []*pb.Entry

	EraseAfter(idx int64) []*pb.Entry

	GetRange(lo, hi int64) []*pb.Entry

	Append(newEnt *pb.Entry)

	GetEntry(idx int64) *pb.Entry

	GetLast() *pb.Entry
}

//
// Mem
//

func MakeMemRaftLog() *RaftLog {
	empEnt := &pb.Entry{}
	newItems := []*pb.Entry{}
	newItems = append(newItems, empEnt)
	return &RaftLog{items: newItems, firstIdx: 0, lastIdx: 1}
}

func (raftlog *RaftLog) GetMemFirst() *pb.Entry {
	return raftlog.items[0]
}

func (raftlog *RaftLog) MemLogItemCount() int {
	return len(raftlog.items)
}

func (raftlog *RaftLog) EraseMemBefore(idx int64) []*pb.Entry {
	raftlog.items = raftlog.items[idx:]
	raftlog.firstIdx = uint64(idx)
	return raftlog.items
}

func (raftlog *RaftLog) EraseMemAfter(idx int64) []*pb.Entry {
	raftlog.items = raftlog.items[:idx]
	raftlog.lastIdx = uint64(idx)
	return raftlog.items
}

func (raftlog *RaftLog) GetMemBefore(idx int64) []*pb.Entry {
	return raftlog.items[:idx+1]
}

func (raftlog *RaftLog) GetMemAfter(idx int64) []*pb.Entry {
	return raftlog.items[idx:]
}

func (raftlog *RaftLog) GetMemRange(lo, hi int64) []*pb.Entry {
	return raftlog.items[lo:hi]
}

func (raftlog *RaftLog) MemAppend(newEnt *pb.Entry) {
	raftlog.items = append(raftlog.items, newEnt)
	raftlog.lastIdx = uint64(newEnt.Index)
}

func (raftlog *RaftLog) GetMemEntry(idx int64) *pb.Entry {
	return raftlog.items[idx]
}

func (raftlog *RaftLog) GetMemLast() *pb.Entry {
	return raftlog.items[raftlog.lastIdx]
}
