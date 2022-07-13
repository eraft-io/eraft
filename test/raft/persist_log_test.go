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
	"io/ioutil"
	"os"
	"path"
	"testing"

	"github.com/eraft-io/eraft/pkg/engine"
	pb "github.com/eraft-io/eraft/pkg/protocol"

	"github.com/eraft-io/eraft/pkg/core/raft"
)

func RemoveDir(in string) {
	dir, _ := ioutil.ReadDir(in)
	for _, d := range dir {
		os.RemoveAll(path.Join([]string{in, d.Name()}...))
	}
}

func TestTestPersisLogGetInit(t *testing.T) {
	newdbEng := engine.KvStoreFactory("leveldb", "./log_data_test")
	raftLog := raft.MakePersistRaftLog(newdbEng)
	fristEnt := raftLog.GetFirst()
	t.Logf("first log %s", fristEnt.String())
	lastEnt := raftLog.GetLast()
	t.Logf("last log %s", lastEnt.String())
	t.Logf("log items count %d", raftLog.LogItemCount())
	RemoveDir("./log_data_test")
}

func TestEraseBefore1(t *testing.T) {
	newdbEng := engine.KvStoreFactory("leveldb", "./log_data_test")
	raftLog := raft.MakePersistRaftLog(newdbEng)
	fristEnt := raftLog.GetFirst()
	t.Logf("first log %s", fristEnt.String())
	lastEnt := raftLog.GetLast()
	t.Logf("last log %s", lastEnt.String())
	ents := raftLog.EraseBefore(1)
	t.Logf("%v", ents)
	RemoveDir("./log_data_test")
}

func TestPersisEraseAfter1(t *testing.T) {
	newdbEng := engine.KvStoreFactory("leveldb", "./log_data_test")
	raftLog := raft.MakePersistRaftLog(newdbEng)
	fristEnt := raftLog.GetFirst()
	t.Logf("first log %s", fristEnt.String())
	lastEnt := raftLog.GetLast()
	t.Logf("last log %s", lastEnt.String())
	ents := raftLog.EraseAfter(1, false)
	t.Logf("%v", ents)
	RemoveDir("./log_data_test")
}

func TestPersisEraseAfter0And1(t *testing.T) {
	newdbEng := engine.KvStoreFactory("leveldb", "./log_data_test")
	raftLog := raft.MakePersistRaftLog(newdbEng)
	fristEnt := raftLog.GetFirst()
	t.Logf("first log %s", fristEnt.String())
	lastEnt := raftLog.GetLast()
	t.Logf("last log %s", lastEnt.String())
	ents := raftLog.EraseAfter(0, false)
	t.Logf("%v", ents)
	raftLog.Append(&pb.Entry{
		Index: 1,
		Term:  1,
	})
	ents = raftLog.EraseAfter(1, false)
	t.Logf("%v", ents)
	t.Logf("%d", raftLog.LogItemCount())
	RemoveDir("./log_data_test")
}

func TestPersisEraseBefore0And1(t *testing.T) {
	newdbEng := engine.KvStoreFactory("leveldb", "./log_data_test")
	raftLog := raft.MakePersistRaftLog(newdbEng)
	fristEnt := raftLog.GetFirst()
	t.Logf("first log %s", fristEnt.String())
	lastEnt := raftLog.GetLast()
	t.Logf("last log %s", lastEnt.String())
	ents := raftLog.EraseBefore(0)
	t.Logf("%v", ents)
	raftLog.Append(&pb.Entry{
		Index: 1,
		Term:  1,
	})
	raftLog.Append(&pb.Entry{
		Index: 2,
		Term:  1,
	})
	ents = raftLog.EraseBefore(1)
	t.Logf("%v", ents)
	t.Logf("%d", raftLog.LogItemCount())
	RemoveDir("./log_data_test")
}

func TestPersisEraseAfter0(t *testing.T) {
	newdbEng := engine.KvStoreFactory("leveldb", "./log_data_test")
	raftLog := raft.MakePersistRaftLog(newdbEng)
	fristEnt := raftLog.GetFirst()
	t.Logf("first log %s", fristEnt.String())
	lastEnt := raftLog.GetLast()
	t.Logf("last log %s", lastEnt.String())
	ents := raftLog.EraseAfter(0, false)
	t.Logf("%v", ents)
	RemoveDir("./log_data_test")
}

func TestTestPersisLogAppend(t *testing.T) {
	newdbEng := engine.KvStoreFactory("leveldb", "./log_data_test")
	raftLog := raft.MakePersistRaftLog(newdbEng)
	for i := 0; i < 1000; i++ {
		raftLog.Append(&pb.Entry{
			Index: int64(i),
			Term:  1,
			Data:  []byte{0x01, 0x02},
		})
	}
	fristEnt := raftLog.GetFirst()
	t.Logf("first log %s", fristEnt.String())
	lastEnt := raftLog.GetLast()
	t.Logf("last log %s", lastEnt.String())
	t.Logf("log items count %d", raftLog.LogItemCount())
	t.Logf("get log item with id 1 -> %s", raftLog.GetEntry(1).String())
	RemoveDir("./log_data_test")
}

func TestTestPersisLogErase(t *testing.T) {
	newdbEng := engine.KvStoreFactory("leveldb", "./log_data_test")
	raftLog := raft.MakePersistRaftLog(newdbEng)
	raftLog.Append(&pb.Entry{
		Index: 1,
		Term:  1,
		Data:  []byte{0x01, 0x02},
	})
	raftLog.Append(&pb.Entry{
		Index: 2,
		Term:  1,
		Data:  []byte{0x01, 0x02},
	})
	raftLog.Append(&pb.Entry{
		Index: 3,
		Term:  1,
		Data:  []byte{0x01, 0x02},
	})
	raftLog.Append(&pb.Entry{
		Index: 4,
		Term:  1,
		Data:  []byte{0x01, 0x02},
	})
	raftLog.EraseBefore(0)
	fristEnt := raftLog.GetFirst()
	t.Logf("first log %s", fristEnt.String())
	lastEnt := raftLog.GetLast()
	t.Logf("last log %s", lastEnt.String())
	t.Logf("log items count %d", raftLog.LogItemCount())
	t.Logf("get log item with id 2 -> %s", raftLog.GetEntry(2).String())
	raftLog.EraseAfter(3, false)
	fristEnt = raftLog.GetFirst()
	t.Logf("first log %s", fristEnt.String())
	lastEnt = raftLog.GetLast()
	t.Logf("last log %s", lastEnt.String())
	t.Logf("get log item with id 3 -> %s", raftLog.GetEntry(3).String())
	RemoveDir("./log_data_test")
}

func TestSliceSplit(t *testing.T) {
	seq := []int{0, 1, 2}
	t.Logf("%+v", seq[1:])
	t.Logf("%+v", seq[:1])
}

func TestRaftStatePersis(t *testing.T) {
	newdbEng := engine.KvStoreFactory("leveldb", "./log_data_test")
	raftLog := raft.MakePersistRaftLog(newdbEng)
	curterm, votedFor := raftLog.ReadRaftState()
	t.Logf("%d", curterm)
	t.Logf("%d", votedFor)
	raftLog.PersistRaftState(5, 5)
	curterm, votedFor = raftLog.ReadRaftState()
	t.Logf("%d", curterm)
	t.Logf("%d", votedFor)
	RemoveDir("./log_data_test")
}

func TestPersisLogGetRange(t *testing.T) {
	newdbEng := engine.KvStoreFactory("leveldb", "./log_data_test")
	raftLog := raft.MakePersistRaftLog(newdbEng)
	raftLog.Append(&pb.Entry{
		Index: 1,
		Term:  1,
		Data:  []byte{0x01, 0x02},
	})
	raftLog.Append(&pb.Entry{
		Index: 2,
		Term:  1,
		Data:  []byte{0x01, 0x02},
	})
	raftLog.Append(&pb.Entry{
		Index: 3,
		Term:  1,
		Data:  []byte{0x01, 0x02},
	})
	raftLog.Append(&pb.Entry{
		Index: 4,
		Term:  1,
		Data:  []byte{0x01, 0x02},
	})

	ents := raftLog.GetRange(2, 3)
	for _, ent := range ents {
		t.Logf("got ent %s", ent.String())
	}
	RemoveDir("./log_data_test")
}

func TestPersisLogGetRangeAfterGc(t *testing.T) {
	newdbEng := engine.KvStoreFactory("leveldb", "./log_data_test")
	raftLog := raft.MakePersistRaftLog(newdbEng)
	raftLog.Append(&pb.Entry{
		Index: 1,
		Term:  1,
		Data:  []byte{0x01, 0x02},
	})
	raftLog.Append(&pb.Entry{
		Index: 2,
		Term:  1,
		Data:  []byte{0x01, 0x02},
	})
	raftLog.Append(&pb.Entry{
		Index: 3,
		Term:  1,
		Data:  []byte{0x01, 0x02},
	})
	raftLog.Append(&pb.Entry{
		Index: 4,
		Term:  1,
		Data:  []byte{0x01, 0x02},
	})
	raftLog.EraseBeforeWithDel(2)
	ents := raftLog.GetRange(1, 2)
	for _, ent := range ents {
		t.Logf("got ent %s", ent.String())
	}
	RemoveDir("./log_data_test")
}
