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

package test

import (
	"testing"

	raftlog "github.com/eraft-io/eraft/pkg/core/raft"
	pb "github.com/eraft-io/eraft/pkg/protocol"
)

func TestMemLogGetInit(t *testing.T) {
	raftLog := raftlog.MakeMemRaftLog()
	t.Log(raftLog.GetMemFirst())
	t.Log(raftLog.GetMemLast())
	t.Log(raftLog.MemLogItemCount())
}

func TestMenEraseBefore1(t *testing.T) {
	raftLog := raftlog.MakeMemRaftLog()
	fristEnt := raftLog.GetMemFirst()
	t.Logf("first log %s", fristEnt.String())
	lastEnt := raftLog.GetMemLast()
	t.Logf("last log %s", lastEnt.String())
	ents := raftLog.EraseMemBefore(1)
	t.Logf("%v", ents)
}

func TestEraseAfter1(t *testing.T) {
	raftLog := raftlog.MakeMemRaftLog()
	fristEnt := raftLog.GetMemFirst()
	t.Logf("first log %s", fristEnt.String())
	lastEnt := raftLog.GetMemLast()
	t.Logf("last log %s", lastEnt.String())
	ents := raftLog.EraseMemAfter(1)
	t.Logf("%v", ents)
}

func TestEraseAfter0And1(t *testing.T) {
	raftLog := raftlog.MakeMemRaftLog()
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
	t.Logf("%d", raftLog.MemLogItemCount())
}

func TestEraseBefore0And1(t *testing.T) {
	raftLog := raftlog.MakeMemRaftLog()
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
	t.Logf("%d", raftLog.MemLogItemCount())
}
