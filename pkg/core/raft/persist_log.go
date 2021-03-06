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
	return &RaftLog{dbEng: newdbEng}
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

func (rfLog *RaftLog) GetEnt(offset int64) *pb.Entry {
	firstLogId := rfLog.GetFirstLogId()
	encodeValue, err := rfLog.dbEng.Get(EncodeRaftLogKey(firstLogId + uint64(offset)))
	if err != nil {
		log.MainLogger.Debug().Msgf("get log entry with id %d error!", offset+int64(firstLogId))
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

// erase after idx, !!!WRANNING!!! is withDel is true, this operation will delete log key
// in storage engine
//
func (rfLog *RaftLog) EraseAfter(idx int64, withDel bool) []*pb.Entry {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	firstLogId := rfLog.GetFirstLogId()
	if withDel {
		for i := int64(firstLogId) + idx; i <= int64(rfLog.GetLastLogId()); i++ {
			if err := rfLog.dbEng.Del(EncodeRaftLogKey(uint64(i))); err != nil {
				panic(err)
			}
		}
	}
	ents := []*pb.Entry{}
	for i := firstLogId; i < firstLogId+uint64(idx); i++ {
		ents = append(ents, rfLog.GetEnt(int64(i)-int64(firstLogId)))
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
	firstLogId := rfLog.GetFirstLogId()
	for i := int64(firstLogId) + idx; i <= int64(lastLogId); i++ {
		ents = append(ents, rfLog.GetEnt(i-int64(firstLogId)))
	}
	return ents
}

func (rfLog *RaftLog) EraseBeforeWithDel(idx int64) error {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	firstLogId := rfLog.GetFirstLogId()
	for i := firstLogId; i < firstLogId+uint64(idx); i++ {
		if err := rfLog.dbEng.Del(EncodeRaftLogKey(i)); err != nil {
			return err
		}
		log.MainLogger.Debug().Msgf("del log with id %d success", i)
	}
	return nil
}

// Append
// append a new entry to raftlog, put it to storage engine
func (rfLog *RaftLog) Append(newEnt *pb.Entry) {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	logIdLast, err := rfLog.dbEng.SeekPrefixKeyIdMax(consts.RAFTLOG_PREFIX)
	if err != nil {
		panic(err)
	}
	newEntEncode := EncodeEntry(newEnt)
	rfLog.dbEng.Put(EncodeRaftLogKey(uint64(logIdLast)+1), newEntEncode)
}

// LogItemCount
// get total log count from storage engine
func (rfLog *RaftLog) LogItemCount() int {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	kBytes, _, err := rfLog.dbEng.SeekPrefixFirst(string(consts.RAFTLOG_PREFIX))
	if err != nil {
		panic(err)
	}
	logIdFirst := DecodeRaftLogKey(kBytes)
	logIdLast, err := rfLog.dbEng.SeekPrefixKeyIdMax(consts.RAFTLOG_PREFIX)
	if err != nil {
		panic(err)
	}
	return int(logIdLast) - int(logIdFirst) + 1
}

// GetLast
//
// get the last entry from storage engine
//
func (rfLog *RaftLog) GetLast() *pb.Entry {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	lastLogId, err := rfLog.dbEng.SeekPrefixKeyIdMax(consts.RAFTLOG_PREFIX)
	if err != nil {
		panic(err)
	}
	firstIdx := rfLog.GetFirstLogId()
	log.MainLogger.Debug().Msgf("get last log with id -> %d", lastLogId)
	return rfLog.GetEnt(int64(lastLogId) - int64(firstIdx))
}

// GetFirst
//
// get the first entry from storage engine
//
func (rfLog *RaftLog) GetFirst() *pb.Entry {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	kBytes, vBytes, err := rfLog.dbEng.SeekPrefixFirst(string(consts.RAFTLOG_PREFIX))
	if err != nil {
		panic(err)
	}
	logId := DecodeRaftLogKey(kBytes)
	log.MainLogger.Debug().Msgf("get first log with id -> %d", logId)
	return DecodeEntry(vBytes)
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
	if err := rfLog.dbEng.Del(EncodeRaftLogKey(firstIdx)); err != nil {
		return err
	}
	ent := DecodeEntry(encodeValue)
	ent.Term = uint64(term)
	ent.Index = index
	log.MainLogger.Debug().Msgf("change first ent to -> " + ent.String())
	newEntEncode := EncodeEntry(ent)
	return rfLog.dbEng.Put(EncodeRaftLogKey(uint64(index)), newEntEncode)
}

// ReInitLogs
// make logs to init state
func (rfLog *RaftLog) ReInitLogs() error {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	// delete all log
	if err := rfLog.dbEng.DelPrefixKeys(string(consts.RAFTLOG_PREFIX)); err != nil {
		return err
	}
	// add a empty
	empEnt := &pb.Entry{}
	empEntEncode := EncodeEntry(empEnt)
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
	kBytes, _, err := rfLog.dbEng.SeekPrefixFirst(string(consts.RAFTLOG_PREFIX))
	if err != nil {
		panic(err)
	}
	return DecodeRaftLogKey(kBytes)
}

// GetLastLogId
// get the last log id from storage engine
func (rfLog *RaftLog) GetLastLogId() uint64 {
	idMax, err := rfLog.dbEng.SeekPrefixKeyIdMax(consts.RAFTLOG_PREFIX)
	if err != nil {
		panic(err)
	}
	return idMax
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
