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
	"sync"

	pb "github.com/eraft-io/eraft/raftpb"
	"github.com/eraft-io/eraft/storage"
)

type RaftLog struct {
	mu       sync.RWMutex
	firstIdx uint64
	lastIdx  uint64
	dbEng    storage.KvStore
	items    []*pb.Entry
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
	return &RaftLog{items: newItems, firstIdx: InitLogIndex, lastIdx: InitLogIndex + 1}
}

func (rfLog *RaftLog) GetMemFirst() *pb.Entry {
	return rfLog.items[0]
}

func (rfLog *RaftLog) MemLogItemCount() int {
	return len(rfLog.items)
}

func (rfLog *RaftLog) EraseMemBefore(idx int64) []*pb.Entry {
	return rfLog.items[idx:]
}

func (rfLog *RaftLog) EraseMemAfter(idx int64) []*pb.Entry {
	return rfLog.items[:idx]
}

func (rfLog *RaftLog) GetMemRange(lo, hi int64) []*pb.Entry {
	return rfLog.items[lo:hi]
}

func (rfLog *RaftLog) MemAppend(newEnt *pb.Entry) {
	rfLog.items = append(rfLog.items, newEnt)
}

func (rfLog *RaftLog) GetMemEntry(idx int64) *pb.Entry {
	return rfLog.items[idx]
}

func (rfLog *RaftLog) GetMemLast() *pb.Entry {
	return rfLog.items[len(rfLog.items)-1]
}
