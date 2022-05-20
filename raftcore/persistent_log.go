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
//
// this file defines the models that raft needs to persist and their operations
//
//
package raftcore

import (
	"bytes"
	"encoding/binary"
	"encoding/gob"
	"fmt"

	pb "github.com/eraft-io/mit6.824lab2product/raftpb"
	"github.com/eraft-io/mit6.824lab2product/storage_eng"
)

type RaftPersistenState struct {
	CurTerm  int64
	VotedFor int64
}

//
// MakePersistRaftLog make a persist raft log model
//
// newdbEng: a LevelDBKvStore storage engine
//
func MakePersistRaftLog(newdbEng storage_eng.KvStore) *RaftLog {
	empEnt := &pb.Entry{}
	empEntEncode := EncodeEntry(empEnt)
	newdbEng.PutBytesKv(EncodeRaftLogKey(INIT_LOG_INDEX), empEntEncode)
	return &RaftLog{dbEng: newdbEng}
}

//
// PersistRaftState Persistent storage raft state
// (curTerm, and votedFor)
// you can find this design in raft paper figure2 State definition
//
func (rfLog *RaftLog) PersistRaftState(curTerm int64, votedFor int64) {
	rfState := &RaftPersistenState{
		CurTerm:  curTerm,
		VotedFor: votedFor,
	}
	rfLog.dbEng.PutBytesKv(RAFT_STATE_KEY, EncodeRaftState(rfState))
}

//
// ReadRaftState
// read the persist curTerm, votedFor for node from storage engine
//
func (rfLog *RaftLog) ReadRaftState() (curTerm int64, votedFor int64) {
	rfBytes, err := rfLog.dbEng.GetBytesValue(RAFT_STATE_KEY)
	if err != nil {
		return 0, -1
	}
	rfState := DecodeRaftState(rfBytes)
	return rfState.CurTerm, rfState.VotedFor
}

//
// GetFirstLogId
// get the first log id from storage engine
//
func (rfLog *RaftLog) GetFirstLogId() uint64 {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	kBytes, _, err := rfLog.dbEng.SeekPrefixFirst(string(RAFTLOG_PREFIX))
	if err != nil {
		panic(err)
	}
	return DecodeRaftLogKey(kBytes)
}

//
// GetLastLogId
//
// get the last log id from storage engine
func (rfLog *RaftLog) GetLastLogId() uint64 {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	kBytes, _, err := rfLog.dbEng.SeekPrefixLast(string(RAFTLOG_PREFIX))
	if err != nil {
		panic(err)
	}
	return DecodeRaftLogKey(kBytes)
}

//
// GetFirst
//
// get the first entry from storage engine
//
func (rfLog *RaftLog) GetFirst() *pb.Entry {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	kBytes, vBytes, err := rfLog.dbEng.SeekPrefixFirst(string(RAFTLOG_PREFIX))
	if err != nil {
		panic(err)
	}
	logId := DecodeRaftLogKey(kBytes)
	PrintDebugLog(fmt.Sprintf("get first log with id -> %d", logId))
	return DecodeEntry(vBytes)
}

//
// GetLast
//
// get the last entry from storage engine
//
func (rfLog *RaftLog) GetLast() *pb.Entry {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	kBytes, vBytes, err := rfLog.dbEng.SeekPrefixLast(string(RAFTLOG_PREFIX))
	if err != nil {
		panic(err)
	}
	logId := DecodeRaftLogKey(kBytes)
	PrintDebugLog(fmt.Sprintf("get last log with id -> %d", logId))
	return DecodeEntry(vBytes)
}

//
// LogItemCount
//
// get total log count from storage engine
//
func (rfLog *RaftLog) LogItemCount() int {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	kBytes, _, err := rfLog.dbEng.SeekPrefixFirst(string(RAFTLOG_PREFIX))
	if err != nil {
		panic(err)
	}
	logIdFirst := DecodeRaftLogKey(kBytes)
	kBytes, _, err = rfLog.dbEng.SeekPrefixLast(string(RAFTLOG_PREFIX))
	if err != nil {
		panic(err)
	}
	logIdLast := DecodeRaftLogKey(kBytes)
	return int(logIdLast) - int(logIdFirst) + 1
}

//
// Append
//
// append a new entry to raftlog, put it to storage engine
//
func (rfLog *RaftLog) Append(newEnt *pb.Entry) {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	kBytes, _, err := rfLog.dbEng.SeekPrefixLast(string(RAFTLOG_PREFIX))
	if err != nil {
		panic(err)
	}
	logIdLast := DecodeRaftLogKey(kBytes)
	newEntEncode := EncodeEntry(newEnt)
	rfLog.dbEng.PutBytesKv(EncodeRaftLogKey(int64(logIdLast)+1), newEntEncode)
}

//
// EraseBefore
// erase log before from idx, and copy [idx:] log return
// this operation don't modity log in storage engine
//
func (rfLog *RaftLog) EraseBefore(idx int64) []*pb.Entry {
	ents := []*pb.Entry{}
	lastLogId := rfLog.GetLastLogId()
	for i := idx; i <= int64(lastLogId); i++ {
		ents = append(ents, rfLog.GetEntry(i))
	}
	return ents
}

//
// EraseAfter
// erase after idx, !!!WRANNING!!! is withDel is true, this operation will delete log key
// in storage engine
//
func (rfLog *RaftLog) EraseAfter(idx int64, withDel bool) []*pb.Entry {
	firstLogId := rfLog.GetFirstLogId()
	if withDel {
		for i := idx; i <= int64(rfLog.GetLastLogId()); i++ {
			if err := rfLog.dbEng.DeleteBytesK(EncodeRaftLogKey(i)); err != nil {
				panic(err)
			}
		}
	}
	ents := []*pb.Entry{}
	for i := firstLogId; i < uint64(idx); i++ {
		ents = append(ents, rfLog.GetEntry(int64(i)))
	}
	return ents
}

//
// GetRange
// get range log from storage engine, and return the copy
// [lo, hi)
//
func (rfLog *RaftLog) GetRange(lo, hi int64) []*pb.Entry {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	ents := []*pb.Entry{}
	for i := lo; i < hi; i++ {
		ents = append(ents, rfLog.GetEntry(i))
	}
	return ents
}

//
// GetEntry
// get log entry with idx
//
func (rfLog *RaftLog) GetEntry(idx int64) *pb.Entry {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	encodeValue, err := rfLog.dbEng.GetBytesValue(EncodeRaftLogKey(idx))
	if err != nil {
		panic(err)
	}
	return DecodeEntry(encodeValue)
}

//
// EncodeRaftLogKey
// encode raft log key with perfix -> RAFTLOG_PREFIX
//
func EncodeRaftLogKey(idx int64) []byte {
	var outBuf bytes.Buffer
	outBuf.Write(RAFTLOG_PREFIX)
	b := make([]byte, 8)
	binary.LittleEndian.PutUint64(b, uint64(idx))
	outBuf.Write(b)
	return outBuf.Bytes()
}

//
// DecodeRaftLogKey
// deocde raft log key, return log id
//
func DecodeRaftLogKey(bts []byte) uint64 {
	return binary.LittleEndian.Uint64(bts[4:])
}

//
// EncodeEntry
// encode log entry to bytes sequence
//
func EncodeEntry(ent *pb.Entry) []byte {
	var bytesEnt bytes.Buffer
	enc := gob.NewEncoder(&bytesEnt)
	enc.Encode(ent)
	return bytesEnt.Bytes()
}

//
// DecodeEntry
// decode log entry from bytes sequence
//
func DecodeEntry(in []byte) *pb.Entry {
	dec := gob.NewDecoder(bytes.NewBuffer(in))
	ent := pb.Entry{}
	dec.Decode(&ent)
	return &ent
}

//
// EncodeRaftState
// encode RaftPersistenState to bytes sequence
//
func EncodeRaftState(rfState *RaftPersistenState) []byte {
	var bytesState bytes.Buffer
	enc := gob.NewEncoder(&bytesState)
	enc.Encode(rfState)
	return bytesState.Bytes()
}

//
// DecodeRaftState
// decode RaftPersistenState from bytes sequence
//
func DecodeRaftState(in []byte) *RaftPersistenState {
	dec := gob.NewDecoder(bytes.NewBuffer(in))
	rfState := RaftPersistenState{}
	dec.Decode(&rfState)
	return &rfState
}
