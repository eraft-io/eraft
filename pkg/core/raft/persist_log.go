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
	"bytes"
	"encoding/binary"
	"encoding/gob"

	"github.com/eraft-io/eraft/pkg/consts"
	"github.com/eraft-io/eraft/pkg/engine"
	"github.com/eraft-io/eraft/pkg/log"
	pb "github.com/eraft-io/eraft/pkg/protocol"
)

type RaftPersistenState struct {
	curTerm  int64
	votedFor int64
}

func MakePersistRaftLog(newdbEng engine.KvStore) *RaftLog {
	empEnt := &pb.Entry{}
	empEntEncode := EncodeEntry(empEnt)
	newdbEng.Put(EncodeRaftLogKey(consts.INIT_LOG_INDEX), empEntEncode)
	return &RaftLog{dbEng: newdbEng, firstIdx: 0, lastIdx: 0}
}

// PersistRaftState Persistent storage raft state
// (curTerm, and votedFor)
// you can find this design in raft paper figure2 State definition
//
func (rfLog *RaftLog) PersistRaftState(curTerm int64, votedFor int64) {
	rfState := &RaftPersistenState{
		curTerm:  curTerm,
		votedFor: votedFor,
	}
	rfLog.dbEng.Put(consts.RAFT_STATE_KEY, EncodeRaftState(rfState))
}

// ReadRaftState
// read the persist curTerm, votedFor for node from storage engine
func (rfLog *RaftLog) ReadRaftState() (curTerm int64, votedFor int64) {
	rfBytes, err := rfLog.dbEng.Get(consts.RAFT_STATE_KEY)
	if err != nil {
		return 0, -1
	}
	rfState := DecodeRaftState(rfBytes)
	return rfState.curTerm, rfState.votedFor
}

func (rfLog *RaftLog) PersisSnapshot(snapContext []byte) {
	rfLog.dbEng.Put(consts.SNAPSHOT_STATE_KEY, snapContext)
	// rfLog.dbEng.GetPrefixRangeKvs(consts.RAFTLOG_PREFIX)
}

func (rfLog *RaftLog) ReadSnapshot() ([]byte, error) {
	bytes, err := rfLog.dbEng.Get(consts.SNAPSHOT_STATE_KEY)
	if err != nil {
		return nil, err
	}
	return bytes, nil
}

// GetEntry
// get log entry with idx
func (rfLog *RaftLog) GetEntry(idx int64) *pb.Entry {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	return rfLog.GetEnt(idx)
}

func (rfLog *RaftLog) GetEnt(index int64) *pb.Entry {
	encodeValue, err := rfLog.dbEng.Get(EncodeRaftLogKey(uint64(index)))
	if err != nil {
		log.MainLogger.Debug().Msgf("get log entry with id %d error! fristlog index is %d, lastlog index is %d\n", int64(index), rfLog.firstIdx, rfLog.lastIdx)
		rfLog.dbEng.GetPrefixRangeKvs(consts.RAFTLOG_PREFIX)
		panic(err)
	}
	return DecodeEntry(encodeValue)
}

// get range log from storage engine, and return the copy
// [lo, hi)
//
func (rfLog *RaftLog) GetRange(lo, hi int64) []*pb.Entry {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	ents := []*pb.Entry{}
	for i := lo; i < hi; i++ {
		ents = append(ents, rfLog.GetEnt(i))
	}
	return ents
}

// erase after idx, !!!WRANNING!!! is withDel is true, this operation will delete log key[idx:]
// in storage engine
//
func (rfLog *RaftLog) EraseAfter(idx int64, withDel bool) []*pb.Entry {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	firstLogId := rfLog.GetFirstLogId()
	log.MainLogger.Debug().Msgf("start erase after %d\n", idx)
	if withDel {
		for i := idx; i <= int64(rfLog.GetLastLogId()); i++ {
			if err := rfLog.dbEng.Del(EncodeRaftLogKey(uint64(i))); err != nil {
				panic(err)
			}
		}
		rfLog.lastIdx = uint64(idx) - 1
	}
	ents := []*pb.Entry{}
	for i := firstLogId; i < uint64(idx); i++ {
		ents = append(ents, rfLog.GetEnt(int64(i)))
	}
	return ents
}

// EraseBefore
// erase log before from idx, and copy [idx:] log return
// this operation don't modity log in storage engine
//
func (rfLog *RaftLog) EraseBefore(idx int64) []*pb.Entry {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	ents := []*pb.Entry{}
	lastLogId := rfLog.GetLastLogId()
	log.MainLogger.Debug().Msgf("Get log [%d:%d] ", idx, lastLogId)
	for i := idx; i <= int64(lastLogId); i++ {
		ents = append(ents, rfLog.GetEnt(i))
	}
	return ents
}

func (rfLog *RaftLog) EraseBeforeWithDel(idx int64) error {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	firstLogId := rfLog.GetFirstLogId()
	for i := firstLogId; i < uint64(idx); i++ {
		if err := rfLog.dbEng.Del(EncodeRaftLogKey(i)); err != nil {
			log.MainLogger.Debug().Msgf("Erase before error\n")
			return err
		}
		log.MainLogger.Debug().Msgf("del log with id %d success", i)
	}
	// rfLog.dbEng.GetPrefixRangeKvs(consts.RAFTLOG_PREFIX)
	rfLog.firstIdx = uint64(idx)
	log.MainLogger.Debug().Msgf("After erase log, firstIdx: %d, lastIdx: %d\n", rfLog.firstIdx, rfLog.lastIdx)
	return nil
}

// Append
// append a new entry to raftlog, put it to storage engine
func (rfLog *RaftLog) Append(newEnt *pb.Entry) {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	newEntEncode := EncodeEntry(newEnt)
	err := rfLog.dbEng.Put(EncodeRaftLogKey(uint64(newEnt.Index)), newEntEncode)
	if err != nil {
		panic(err)
	}
	if newEnt.Index > int64(rfLog.lastIdx) {
		rfLog.lastIdx = uint64(newEnt.Index)
	}
	log.MainLogger.Debug().Msgf("Append entry index:%d to levebdb log\n", newEnt.Index)
}

// LogItemCount
// get total log count from storage engine
func (rfLog *RaftLog) LogItemCount() int {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	return int(rfLog.lastIdx) - int(rfLog.firstIdx) + 1
}

// GetLast
//
// get the last entry from storage engine
//
func (rfLog *RaftLog) GetLast() *pb.Entry {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	return rfLog.GetEnt(int64(rfLog.lastIdx))
}

// GetFirst
//
// get the first entry from storage engine
//
func (rfLog *RaftLog) GetFirst() *pb.Entry {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	return rfLog.GetEnt(int64(rfLog.firstIdx))
}

// SetEntFirstTermAndIndex
//

func (rfLog *RaftLog) SetEntFirstTermAndIndex(term, index int64) error {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	firstIdx := rfLog.GetFirstLogId()
	encodeValue, err := rfLog.dbEng.Get(EncodeRaftLogKey(uint64(firstIdx)))
	if err != nil {
		log.MainLogger.Debug().Msgf("get log entry with id %d error!", firstIdx)
		panic(err)
	}
	// del olf first ent
	log.MainLogger.Debug().Msgf("del log index:%d\n", firstIdx)
	if err := rfLog.dbEng.Del(EncodeRaftLogKey(firstIdx)); err != nil {
		return err
	}
	ent := DecodeEntry(encodeValue)
	ent.Term = uint64(term)
	ent.Index = index
	log.MainLogger.Debug().Msgf("change first ent to -> " + ent.String())
	newEntEncode := EncodeEntry(ent)
	rfLog.firstIdx, rfLog.lastIdx = uint64(index), uint64(index)
	return rfLog.dbEng.Put(EncodeRaftLogKey(uint64(index)), newEntEncode)
}

// ReInitLogs
// make logs to init state
func (rfLog *RaftLog) ReInitLogs() error {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	log.MainLogger.Debug().Msgf("start reinitlogs\n")
	// delete all log
	if err := rfLog.dbEng.DelPrefixKeys(string(consts.RAFTLOG_PREFIX)); err != nil {
		return err
	}
	// add a empty
	empEnt := &pb.Entry{}
	empEntEncode := EncodeEntry(empEnt)
	rfLog.firstIdx, rfLog.lastIdx = 0, 0
	return rfLog.dbEng.Put(EncodeRaftLogKey(consts.INIT_LOG_INDEX), empEntEncode)
}

// SetEntFirstData
//

func (rfLog *RaftLog) SetEntFirstData(d []byte) error {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	firstIdx := rfLog.GetFirstLogId()
	encodeValue, err := rfLog.dbEng.Get(EncodeRaftLogKey(uint64(firstIdx)))
	if err != nil {
		log.MainLogger.Debug().Msgf("get log entry with id %d error!", firstIdx)
		panic(err)
	}
	ent := DecodeEntry(encodeValue)
	ent.Index = int64(firstIdx)
	ent.Data = d
	newEntEncode := EncodeEntry(ent)
	return rfLog.dbEng.Put(EncodeRaftLogKey(firstIdx), newEntEncode)
}

// GetFirstLogId
// get the first log id from storage engine
func (rfLog *RaftLog) GetFirstLogId() uint64 {
	return rfLog.firstIdx
}

// GetLastLogId
// get the last log id from storage engine
func (rfLog *RaftLog) GetLastLogId() uint64 {
	return rfLog.lastIdx
}

// EncodeRaftLogKey
// encode raft log key with perfix -> RAFTLOG_PREFIX
//
func EncodeRaftLogKey(idx uint64) []byte {
	var outBuf bytes.Buffer
	outBuf.Write(consts.RAFTLOG_PREFIX)
	b := make([]byte, 8)
	binary.BigEndian.PutUint64(b, uint64(idx))
	outBuf.Write(b)
	return outBuf.Bytes()
}

// DecodeRaftLogKey
// deocde raft log key, return log id
func DecodeRaftLogKey(bts []byte) uint64 {
	return binary.BigEndian.Uint64(bts[4:])
}

// EncodeEntry
// encode log entry to bytes sequence
func EncodeEntry(ent *pb.Entry) []byte {
	var bytesEnt bytes.Buffer
	enc := gob.NewEncoder(&bytesEnt)
	enc.Encode(ent)
	return bytesEnt.Bytes()
}

// DecodeEntry
// decode log entry from bytes sequence
func DecodeEntry(in []byte) *pb.Entry {
	dec := gob.NewDecoder(bytes.NewBuffer(in))
	ent := pb.Entry{}
	dec.Decode(&ent)
	return &ent
}

// EncodeRaftState
// encode RaftPersistenState to bytes sequence
func EncodeRaftState(rfState *RaftPersistenState) []byte {
	var bytesState bytes.Buffer
	enc := gob.NewEncoder(&bytesState)
	enc.Encode(rfState)
	return bytesState.Bytes()
}

// DecodeRaftState
// decode RaftPersistenState from bytes sequence
func DecodeRaftState(in []byte) *RaftPersistenState {
	dec := gob.NewDecoder(bytes.NewBuffer(in))
	rfState := RaftPersistenState{}
	dec.Decode(&rfState)
	return &rfState
}
