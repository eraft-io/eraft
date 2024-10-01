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
	"fmt"

	pb "github.com/eraft-io/eraft/raftpb"
	"github.com/eraft-io/eraft/storage"
)

type RaftPersistentState struct {
	CurTerm    int64
	VotedFor   int64
	AppliedIdx int64
}

// MakePersistRaftLog make a persist raft log model
//
// newDBEng: a LevelDBKvStore storage engine
func MakePersistRaftLog(newDBEng storage.KvStore) *RaftLog {
	_, _, err := newDBEng.SeekPrefixFirst(string(RaftLogPrefix))
	if err != nil {
		empEnt := &pb.Entry{}
		empEntEncode := EncodeEntry(empEnt)
		newDBEng.PutBytesKv(EncodeRaftLogKey(InitLogIndex), empEntEncode)
	}
	return &RaftLog{dbEng: newDBEng}
}

// PersistRaftState Persistent storage raft state
// (curTerm, and votedFor)
// you can find this design in raft paper figure2 State definition
func (rfLog *RaftLog) PersistRaftState(curTerm int64, votedFor int64, appliedIdx int64) {
	rfState := &RaftPersistentState{
		CurTerm:    curTerm,
		VotedFor:   votedFor,
		AppliedIdx: appliedIdx,
	}
	rfLog.dbEng.PutBytesKv(RaftStateKey, EncodeRaftState(rfState))
}

func (rfLog *RaftLog) PersisSnapshot(snapContext []byte) {
	rfLog.dbEng.PutBytesKv(SnapshotStateKey, snapContext)
}

func (rfLog *RaftLog) ReadSnapshot() ([]byte, error) {
	value, err := rfLog.dbEng.GetBytesValue(SnapshotStateKey)
	if err != nil {
		return nil, err
	}
	return value, nil
}

// ReadRaftState
// read the persisted curTerm, votedFor for node from storage engine
func (rfLog *RaftLog) ReadRaftState() (curTerm int64, votedFor int64, appliedIdx int64) {
	rfBytes, err := rfLog.dbEng.GetBytesValue(RaftStateKey)
	if err != nil {
		return 0, -1, -1
	}
	rfState := DecodeRaftState(rfBytes)
	return rfState.CurTerm, rfState.VotedFor, rfState.AppliedIdx
}

// GetFirstLogId
// get the first log id from storage engine
func (rfLog *RaftLog) GetFirstLogId() uint64 {
	kBytes, _, err := rfLog.dbEng.SeekPrefixFirst(string(RaftLogPrefix))
	if err != nil {
		panic(err)
	}
	return DecodeRaftLogKey(kBytes)
}

// GetLastLogId
//
// get the last log id from storage engine
func (rfLog *RaftLog) GetLastLogId() uint64 {
	idMax, err := rfLog.dbEng.SeekPrefixKeyIdMax(RaftLogPrefix)
	if err != nil {
		panic(err)
	}
	return idMax
}

//
// SetEntFirstData
//

func (rfLog *RaftLog) SetEntFirstData(d []byte) error {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	firstIdx := rfLog.GetFirstLogId()
	encodeValue, err := rfLog.dbEng.GetBytesValue(EncodeRaftLogKey(firstIdx))
	if err != nil {
		PrintDebugLog(fmt.Sprintf("get log entry with id %d error!", firstIdx))
		return err
	}
	ent := DecodeEntry(encodeValue)
	ent.Index = int64(firstIdx)
	ent.Data = d
	newEntEncode := EncodeEntry(ent)
	return rfLog.dbEng.PutBytesKv(EncodeRaftLogKey(firstIdx), newEntEncode)
}

// ReInitLogs
// make logs to init state
func (rfLog *RaftLog) ReInitLogs() error {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	// delete all log
	if err := rfLog.dbEng.DelPrefixKeys(string(RaftLogPrefix)); err != nil {
		return err
	}
	// add an empty
	empEnt := &pb.Entry{}
	empEntEncode := EncodeEntry(empEnt)
	return rfLog.dbEng.PutBytesKv(EncodeRaftLogKey(InitLogIndex), empEntEncode)
}

//
// SetEntFirstTermAndIndex
//

func (rfLog *RaftLog) SetEntFirstTermAndIndex(term, index int64) error {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	firstIdx := rfLog.GetFirstLogId()
	encodeValue, err := rfLog.dbEng.GetBytesValue(EncodeRaftLogKey(firstIdx))
	if err != nil {
		PrintDebugLog(fmt.Sprintf("get log entry with id %d error!", firstIdx))
		panic(err)
	}
	// del olf first ent
	if err := rfLog.dbEng.DeleteBytesK(EncodeRaftLogKey(firstIdx)); err != nil {
		return err
	}
	ent := DecodeEntry(encodeValue)
	ent.Term = uint64(term)
	ent.Index = index
	PrintDebugLog("change first ent to -> " + ent.String())
	newEntEncode := EncodeEntry(ent)
	return rfLog.dbEng.PutBytesKv(EncodeRaftLogKey(uint64(index)), newEntEncode)
}

// GetFirst
//
// get the first entry from storage engine
func (rfLog *RaftLog) GetFirst() *pb.Entry {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	kBytes, vBytes, err := rfLog.dbEng.SeekPrefixFirst(string(RaftLogPrefix))
	if err != nil {
		panic(err)
	}
	logId := DecodeRaftLogKey(kBytes)
	PrintDebugLog(fmt.Sprintf("get first log with id -> %d", logId))
	return DecodeEntry(vBytes)
}

// func (rfLog *RaftLog) SetFirstLog(index int64, term int64) error {
// 	rfLog.mu.Lock()
// 	defer rfLog.mu.Unlock()
// 	empEnt := &pb.Entry{
// 		Index: index,
// 		Term:  uint64(term),
// 		Data:  make([]byte, 0),
// 	}
// 	empEntEncode := EncodeEntry(empEnt)
// 	return rfLog.dbEng.PutBytesKv(EncodeRaftLogKey(uint64(index)), empEntEncode)
// }

// GetLast
//
// get the last entry from storage engine
func (rfLog *RaftLog) GetLast() *pb.Entry {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	lastLogId, err := rfLog.dbEng.SeekPrefixKeyIdMax(RaftLogPrefix)
	if err != nil {
		panic(err)
	}
	firstIdx := rfLog.GetFirstLogId()
	PrintDebugLog(fmt.Sprintf("get last log with id -> %d", lastLogId))
	return rfLog.GetEnt(int64(lastLogId) - int64(firstIdx))
}

// LogItemCount
//
// get total log count from storage engine
func (rfLog *RaftLog) LogItemCount() int {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	kBytes, _, err := rfLog.dbEng.SeekPrefixFirst(string(RaftLogPrefix))
	if err != nil {
		panic(err)
	}
	logIdFirst := DecodeRaftLogKey(kBytes)
	logIdLast, err := rfLog.dbEng.SeekPrefixKeyIdMax(RaftLogPrefix)
	if err != nil {
		panic(err)
	}
	return int(logIdLast) - int(logIdFirst) + 1
}

// Append
//
// append a new entry to raft logs, put it to storage engine
func (rfLog *RaftLog) Append(newEnt *pb.Entry) {
	// rfLog.mu.Lock()
	// defer rfLog.mu.Unlock()
	logIdLast, err := rfLog.dbEng.SeekPrefixKeyIdMax(RaftLogPrefix)
	if err != nil {
		panic(err)
	}
	newEntEncode := EncodeEntry(newEnt)
	rfLog.dbEng.PutBytesKv(EncodeRaftLogKey(uint64(logIdLast)+1), newEntEncode)
}

// EraseBefore
// erase log before from idx, and copy [idx:] log return
// this operation don't modify log in storage engine
func (rfLog *RaftLog) EraseBefore(idx int64) []*pb.Entry {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	entries := []*pb.Entry{}
	lastLogId := rfLog.GetLastLogId()
	firstLogId := rfLog.GetFirstLogId()
	for i := int64(firstLogId) + idx; i <= int64(lastLogId); i++ {
		entries = append(entries, rfLog.GetEnt(i-int64(firstLogId)))
	}
	return entries
}

func (rfLog *RaftLog) EraseBeforeWithDel(idx int64) error {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	firstLogId := rfLog.GetFirstLogId()
	for i := firstLogId; i < firstLogId+uint64(idx); i++ {
		if err := rfLog.dbEng.DeleteBytesK(EncodeRaftLogKey(i)); err != nil {
			return err
		}
		PrintDebugLog(fmt.Sprintf("del log with id %d success", i))
	}
	return nil
}

// EraseAfter
// erase after idx, !!!WARNING!!! is withDel is true, this operation will delete log key
// in storage engine
func (rfLog *RaftLog) EraseAfter(idx int64, withDel bool) []*pb.Entry {
	rfLog.mu.Lock()
	defer rfLog.mu.Unlock()
	firstLogId := rfLog.GetFirstLogId()
	if withDel {
		for i := int64(firstLogId) + idx; i <= int64(rfLog.GetLastLogId()); i++ {
			if err := rfLog.dbEng.DeleteBytesK(EncodeRaftLogKey(uint64(i))); err != nil {
				panic(err)
			}
		}
	}
	entries := []*pb.Entry{}
	for i := firstLogId; i < firstLogId+uint64(idx); i++ {
		entries = append(entries, rfLog.GetEnt(int64(i)-int64(firstLogId)))
	}
	return entries
}

// GetRange
// get range log from storage engine, and return the copy
// [lo, hi)
func (rfLog *RaftLog) GetRange(lo, hi int64) []*pb.Entry {
	rfLog.mu.RLock()
	defer rfLog.mu.RUnlock()
	ents := []*pb.Entry{}
	for i := lo; i < hi; i++ {
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

func (rfLog *RaftLog) GetEnt(offset int64) *pb.Entry {
	firstLogId := rfLog.GetFirstLogId()
	encodeValue, err := rfLog.dbEng.GetBytesValue(EncodeRaftLogKey(firstLogId + uint64(offset)))
	if err != nil {
		PrintDebugLog(fmt.Sprintf("get log entry with id %d error!", offset+int64(firstLogId)))
		panic(err)
	}
	return DecodeEntry(encodeValue)
}

// EncodeRaftLogKey
// encode raft log key with prefix -> RaftLogPrefix
func EncodeRaftLogKey(idx uint64) []byte {
	var outBuf bytes.Buffer
	outBuf.Write(RaftLogPrefix)
	b := make([]byte, 8)
	binary.LittleEndian.PutUint64(b, idx)
	outBuf.Write(b)
	return outBuf.Bytes()
}

// DecodeRaftLogKey
// decode raft log key, return log id
func DecodeRaftLogKey(bts []byte) uint64 {
	return binary.LittleEndian.Uint64(bts[4:])
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
// encode RaftPersistentState to bytes sequence
func EncodeRaftState(rfState *RaftPersistentState) []byte {
	var bytesState bytes.Buffer
	enc := gob.NewEncoder(&bytesState)
	enc.Encode(rfState)
	return bytesState.Bytes()
}

// DecodeRaftState
// decode RaftPersistentState from bytes sequence
func DecodeRaftState(in []byte) *RaftPersistentState {
	dec := gob.NewDecoder(bytes.NewBuffer(in))
	rfState := RaftPersistentState{}
	dec.Decode(&rfState)
	return &rfState
}
