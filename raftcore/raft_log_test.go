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
	"testing"

	pb "github.com/eraft-io/mit6.824lab2product/raftpb"
)

func TestEncodeLogKey(t *testing.T) {
	encodeData := EncodeRaftLogKey(10)
	t.Log(encodeData)
	t.Log(DecodeRaftLogKey(encodeData))
}

func TestMemLogGetInit(t *testing.T) {
	raftLog := MakeMemRaftLog()
	t.Log(raftLog.GetMemFirst())
	t.Log(raftLog.GetMemLast())
	t.Log(len(raftLog.items))
	RemoveDir("./log_data_test")
}

func TestMenEraseBefore1(t *testing.T) {
	raftLog := MakeMemRaftLog()
	fristEnt := raftLog.GetMemFirst()
	t.Logf("first log %s", fristEnt.String())
	lastEnt := raftLog.GetMemLast()
	t.Logf("last log %s", lastEnt.String())
	ents := raftLog.EraseMemBefore(1)
	t.Logf("%v", ents)
}

func TestEraseAfter1(t *testing.T) {
	raftLog := MakeMemRaftLog()
	fristEnt := raftLog.GetMemFirst()
	t.Logf("first log %s", fristEnt.String())
	lastEnt := raftLog.GetMemLast()
	t.Logf("last log %s", lastEnt.String())
	ents := raftLog.EraseMemAfter(1)
	t.Logf("%v", ents)
}

func TestEraseAfter0And1(t *testing.T) {
	raftLog := MakeMemRaftLog()
	fristEnt := raftLog.GetMemFirst()
	t.Logf("first log %s", fristEnt.String())
	lastEnt := raftLog.GetMemLast()
	t.Logf("last log %s", lastEnt.String())
	ents := raftLog.EraseMemAfter(0)
	t.Logf("%v", ents)
	raftLog.MemAppend(&pb.Entry{
		Index: 1,
		Term:  1,
	})
	ents = raftLog.EraseMemAfter(1)
	t.Logf("%v", ents)
	t.Logf("%d", len(raftLog.items))
}

func TestEraseBefore0And1(t *testing.T) {
	raftLog := MakeMemRaftLog()
	fristEnt := raftLog.GetMemFirst()
	t.Logf("first log %s", fristEnt.String())
	lastEnt := raftLog.GetMemLast()
	t.Logf("last log %s", lastEnt.String())
	ents := raftLog.EraseMemBefore(0)
	t.Logf("%v", ents)
	raftLog.MemAppend(&pb.Entry{
		Index: 1,
		Term:  1,
	})
	raftLog.MemAppend(&pb.Entry{
		Index: 2,
		Term:  1,
	})
	ents = raftLog.EraseMemBefore(1)
	t.Logf("%v", ents)
	t.Logf("%d", len(raftLog.items))
}
