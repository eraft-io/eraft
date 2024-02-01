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
// this file defines the models that raft needs to persist and their operations
package raftcore

import (
	"bytes"
	"encoding/binary"
	"encoding/gob"
	"sync"

	"github.com/eraft-io/eraft/logger"
	pb "github.com/eraft-io/eraft/raftpb"
	storage_eng "github.com/eraft-io/eraft/storage"
)

type RaftLog struct {
	mu       sync.RWMutex
	firstIdx uint64
	lastIdx  uint64
	dbEng    storage_eng.KvStore
}

type RaftPersistenState struct {
	CurTerm  int64
	VotedFor int64
}

// MakePersistRaftLog make a persist raft log model
//
// newdbEng: a LevelDBKvStore storage engine
func MakePersistRaftLog(newdbEng storage_eng.KvStore) *RaftLog {
	_, err := newdbEng.GetBytesValue(EncodeRaftLogKey(INIT_LOG_INDEX))
	if err != nil {
		logger.ELogger().Sugar().Debugf("init raft log state")
		emp_ent := &pb.Entry{}
		emp_ent_encode := EncodeEntry(emp_ent)
		newdbEng.PutBytesKv(EncodeRaftLogKey(INIT_LOG_INDEX), emp_ent_encode)
		return &RaftLog{dbEng: newdbEng}
	}
	lidkBytes, _, err := newdbEng.SeekPrefixLast(RAFTLOG_PREFIX)
	if err != nil {
		panic(err)
	}
	last_idx := binary.BigEndian.Uint64(lidkBytes[len(RAFTLOG_PREFIX):])
	fidkBytes, _, err := newdbEng.SeekPrefixFirst(RAFTLOG_PREFIX)
	if err != nil {
		panic(err)
	}
	first_idx := binary.BigEndian.Uint64(fidkBytes[len(RAFTLOG_PREFIX):])
	return &RaftLog{dbEng: newdbEng, lastIdx: last_idx, firstIdx: first_idx}
}

// PersistRaftState Persistent storage raft state
// (curTerm, and votedFor)
// you can find this design in raft paper figure2 State definition
func (rfLog *RaftLog) PersistRaftState(curTerm int64, votedFor int64) {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	rf_state := &RaftPersistenState{
		CurTerm:  curTerm,
		VotedFor: votedFor,
	}
	rfLog.dbEng.PutBytesKv(RAFT_STATE_KEY, EncodeRaftState(rf_state))
}

// ReadRaftState
// read the persist curTerm, votedFor for node from storage engine
func (rfLog *RaftLog) ReadRaftState() (curTerm int64, votedFor int64) {
	rf_bytes, err := rfLog.dbEng.GetBytesValue(RAFT_STATE_KEY)
	if err != nil {
		return 0, -1
	}
	rf_state := DecodeRaftState(rf_bytes)
	return rf_state.CurTerm, rf_state.VotedFor
}

func (rfLog *RaftLog) PersisSnapshot(snapContext []byte) {
	rfLog.dbEng.PutBytesKv(SNAPSHOT_STATE_KEY, snapContext)
}

func (rfLog *RaftLog) ReadSnapshot() ([]byte, error) {
	bytes, err := rfLog.dbEng.GetBytesValue(SNAPSHOT_STATE_KEY)
	if err != nil {
		return nil, err
	}
	return bytes, nil
}

// GetFirstLogId
// get the first log id from storage engine
func (rfLog *RaftLog) GetFirstLogId() uint64 {
	return rfLog.firstIdx
}

// GetLastLogId
//
// get the last log id from storage engine
func (rfLog *RaftLog) GetLastLogId() uint64 {
	return rfLog.lastIdx
}

//
// SetEntFirstData
//

func (rfLog *RaftLog) SetEntFirstData(d []byte) error {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	first_idx := rfLog.GetFirstLogId()
	encode_value, err := rfLog.dbEng.GetBytesValue(EncodeRaftLogKey(uint64(first_idx)))
	if err != nil {
		logger.ELogger().Sugar().Panicf("get log entry with id %d error!", first_idx)
		panic(err)
	}
	ent := DecodeEntry(encode_value)
	ent.Index = int64(first_idx)
	ent.Data = d
	newent_encode := EncodeEntry(ent)
	return rfLog.dbEng.PutBytesKv(EncodeRaftLogKey(first_idx), newent_encode)
}

// ReInitLogs
// make logs to init state
func (rfLog *RaftLog) ReInitLogs() error {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	// delete all log
	if err := rfLog.dbEng.DelPrefixKeys(RAFTLOG_PREFIX); err != nil {
		return err
	}
	rfLog.firstIdx = 0
	rfLog.lastIdx = 0
	// add a empty
	emp_ent := &pb.Entry{}
	empent_encode := EncodeEntry(emp_ent)
	return rfLog.dbEng.PutBytesKv(EncodeRaftLogKey(INIT_LOG_INDEX), empent_encode)
}

//
// SetEntFirstTermAndIndex
//

func (rfLog *RaftLog) ResetFirstEntryTermAndIndex(term, index int64) error {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	first_idx := rfLog.GetFirstLogId()
	encode_value, err := rfLog.dbEng.GetBytesValue(EncodeRaftLogKey(uint64(first_idx)))
	if err != nil {
		logger.ELogger().Sugar().Panicf("get log entry with id %d error!", first_idx)
		panic(err)
	}
	// del olf first ent
	if err := rfLog.dbEng.DeleteBytesK(EncodeRaftLogKey(first_idx)); err != nil {
		return err
	}
	ent := DecodeEntry(encode_value)
	ent.Term = uint64(term)
	ent.Index = index
	rfLog.firstIdx = uint64(index)
	logger.ELogger().Sugar().Debugf("change first ent to -> ", ent.String())
	new_ent_encode := EncodeEntry(ent)
	return rfLog.dbEng.PutBytesKv(EncodeRaftLogKey(uint64(index)), new_ent_encode)
}

// GetFirst
//
// get the first entry from storage engine
func (rfLog *RaftLog) GetFirst() *pb.Entry {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	return rfLog.GetEnt(int64(rfLog.firstIdx))
}

// GetLast
//
// get the last entry from storage engine
func (rfLog *RaftLog) GetLast() *pb.Entry {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	return rfLog.GetEnt(int64(rfLog.lastIdx))
}

// LogItemCount
//
// get total log count from storage engine
func (rfLog *RaftLog) LogItemCount() int {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	return int(rfLog.lastIdx) - int(rfLog.firstIdx) + 1
}

// Append
//
// append a new entry to raftlog, put it to storage engine
func (rfLog *RaftLog) Append(newEnt *pb.Entry) {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	newent_encode := EncodeEntry(newEnt)
	rfLog.dbEng.PutBytesKv(EncodeRaftLogKey(uint64(rfLog.lastIdx)+1), newent_encode)
	rfLog.lastIdx += 1
}

// EraseBefore
// erase log before from idx, and copy [idx:] log return
// this operation don't modity log in storage engine
func (rfLog *RaftLog) EraseBefore(logidx int64, withDel bool) ([]*pb.Entry, error) {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	ents := []*pb.Entry{}
	lastlog_id := rfLog.GetLastLogId()
	firstlog_id := rfLog.GetFirstLogId()
	if withDel {
		for i := firstlog_id; i < uint64(logidx); i++ {
			if err := rfLog.dbEng.DeleteBytesK(EncodeRaftLogKey(i)); err != nil {
				return ents, err
			}
			logger.ELogger().Sugar().Debugf("del log with id %d success", i)
		}
		rfLog.firstIdx = uint64(logidx)
	}
	for i := logidx; i <= int64(lastlog_id); i++ {
		ents = append(ents, rfLog.GetEnt(i))
	}
	return ents, nil
}

// EraseAfter
// erase after idx, !!!WRANNING!!! is withDel is true, this operation will delete log key
// in storage engine
func (rfLog *RaftLog) EraseAfter(logidx int64, withDel bool) []*pb.Entry {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	firstlog_id := rfLog.GetFirstLogId()
	if withDel {
		for i := logidx; i <= int64(rfLog.GetLastLogId()); i++ {
			if err := rfLog.dbEng.DeleteBytesK(EncodeRaftLogKey(uint64(i))); err != nil {
				panic(err)
			}
		}
		rfLog.lastIdx = uint64(logidx) - 1
	}
	ents := []*pb.Entry{}
	for i := firstlog_id; i < uint64(logidx); i++ {
		ents = append(ents, rfLog.GetEnt(int64(i)))
	}
	return ents
}

// GetRange
// get range log from storage engine, and return the copy
// [lo, hi)
func (rfLog *RaftLog) GetRange(lo, hi int64) []*pb.Entry {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	ents := []*pb.Entry{}
	for i := lo; i <= hi; i++ {
		ents = append(ents, rfLog.GetEnt(i))
	}
	return ents
}

// GetEntry
// get log entry with idx
func (rfLog *RaftLog) GetEntry(idx int64) *pb.Entry {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	return rfLog.GetEnt(idx)
}

func (rfLog *RaftLog) GetEnt(logidx int64) *pb.Entry {
	encode_value, err := rfLog.dbEng.GetBytesValue(EncodeRaftLogKey(uint64(logidx)))
	if err != nil {
		logger.ELogger().Sugar().Debugf("get log entry with id %d error!", logidx)
		panic(err)
	}
	return DecodeEntry(encode_value)
}

// EncodeRaftLogKey
// encode raft log key with perfix -> RAFTLOG_PREFIX
func EncodeRaftLogKey(idx uint64) []byte {
	var out_buf bytes.Buffer
	out_buf.Write(RAFTLOG_PREFIX)
	b := make([]byte, 8)
	binary.BigEndian.PutUint64(b, uint64(idx))
	out_buf.Write(b)
	return out_buf.Bytes()
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
	var bytes_state bytes.Buffer
	enc := gob.NewEncoder(&bytes_state)
	enc.Encode(rfState)
	return bytes_state.Bytes()
}

// DecodeRaftState
// decode RaftPersistenState from bytes sequence
func DecodeRaftState(in []byte) *RaftPersistenState {
	dec := gob.NewDecoder(bytes.NewBuffer(in))
	rf_state := RaftPersistenState{}
	dec.Decode(&rf_state)
	return &rf_state
}
