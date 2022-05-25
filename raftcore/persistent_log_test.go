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
	"io/ioutil"
	"os"
	"path"
	"testing"

	pb "github.com/eraft-io/mit6.824lab2product/raftpb"

	"github.com/eraft-io/mit6.824lab2product/storage_eng"
)

func RemoveDir(in string) {
	dir, _ := ioutil.ReadDir(in)
	for _, d := range dir {
		os.RemoveAll(path.Join([]string{in, d.Name()}...))
	}
}

func TestTestPersisLogGetInit(t *testing.T) {
	// newdbEng, err := storage_eng.MakeLevelDBKvStore("./log_data_test")
	// if err != nil {
	// 	PrintDebugLog("boot storage engine err!")
	// 	panic(err)
	// }
	newdbEng := storage_eng.EngineFactory("leveldb", "./log_data_test")
	raftLog := MakePersistRaftLog(newdbEng)
	fristEnt := raftLog.GetFirst()
	t.Logf("first log %s", fristEnt.String())
	lastEnt := raftLog.GetLast()
	t.Logf("last log %s", lastEnt.String())
	t.Logf("log items count %d", raftLog.LogItemCount())
	RemoveDir("./log_data_test")
}

func TestEraseBefore1(t *testing.T) {
	newdbEng := storage_eng.EngineFactory("leveldb", "./log_data_test")
	raftLog := MakePersistRaftLog(newdbEng)
	fristEnt := raftLog.GetFirst()
	t.Logf("first log %s", fristEnt.String())
	lastEnt := raftLog.GetLast()
	t.Logf("last log %s", lastEnt.String())
	ents := raftLog.EraseBefore(1)
	t.Logf("%v", ents)
	RemoveDir("./log_data_test")
}

func TestPersisEraseAfter1(t *testing.T) {
	newdbEng := storage_eng.EngineFactory("leveldb", "./log_data_test")
	raftLog := MakePersistRaftLog(newdbEng)
	fristEnt := raftLog.GetFirst()
	t.Logf("first log %s", fristEnt.String())
	lastEnt := raftLog.GetLast()
	t.Logf("last log %s", lastEnt.String())
	ents := raftLog.EraseAfter(1, false)
	t.Logf("%v", ents)
	RemoveDir("./log_data_test")
}

func TestPersisEraseAfter0And1(t *testing.T) {
	newdbEng := storage_eng.EngineFactory("leveldb", "./log_data_test")
	raftLog := MakePersistRaftLog(newdbEng)
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
	newdbEng := storage_eng.EngineFactory("leveldb", "./log_data_test")
	raftLog := MakePersistRaftLog(newdbEng)
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
	newdbEng := storage_eng.EngineFactory("leveldb", "./log_data_test")
	raftLog := MakePersistRaftLog(newdbEng)
	fristEnt := raftLog.GetFirst()
	t.Logf("first log %s", fristEnt.String())
	lastEnt := raftLog.GetLast()
	t.Logf("last log %s", lastEnt.String())
	ents := raftLog.EraseAfter(0, false)
	t.Logf("%v", ents)
	RemoveDir("./log_data_test")
}

func TestTestPersisLogAppend(t *testing.T) {
	newdbEng := storage_eng.EngineFactory("leveldb", "./log_data_test")
	raftLog := MakePersistRaftLog(newdbEng)
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
	newdbEng := storage_eng.EngineFactory("leveldb", "./log_data_test")
	raftLog := MakePersistRaftLog(newdbEng)
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
	newdbEng := storage_eng.EngineFactory("leveldb", "./log_data_test")
	raftLog := MakePersistRaftLog(newdbEng)
	curterm, votedFor := raftLog.ReadRaftState()
	t.Logf("%d", curterm)
	t.Logf("%d", votedFor)
	raftLog.PersistRaftState(5, 5)
	curterm, votedFor = raftLog.ReadRaftState()
	t.Logf("%d", curterm)
	t.Logf("%d", votedFor)
	RemoveDir("./log_data_test")
}

func TestTestPersisLogGetRange(t *testing.T) {
	newdbEng := storage_eng.EngineFactory("leveldb", "./log_data_test")
	raftLog := MakePersistRaftLog(newdbEng)
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
