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
	"bytes"
	"sync"
	"testing"

	pb "github.com/eraft-io/eraft/raftpb"
	"github.com/stretchr/testify/assert"
)

// MockKvStore implements storage.KvStore interface for testing
type MockKvStore struct {
	data map[string][]byte
	mu   sync.RWMutex
}

func NewMockKvStore() *MockKvStore {
	return &MockKvStore{
		data: make(map[string][]byte),
	}
}

func (m *MockKvStore) PutBytesKv(k []byte, v []byte) error {
	m.mu.Lock()
	defer m.mu.Unlock()
	m.data[string(k)] = v
	return nil
}

func (m *MockKvStore) DeleteBytesK(k []byte) error {
	m.mu.Lock()
	defer m.mu.Unlock()
	delete(m.data, string(k))
	return nil
}

func (m *MockKvStore) GetBytesValue(k []byte) ([]byte, error) {
	m.mu.RLock()
	defer m.mu.RUnlock()
	value, exists := m.data[string(k)]
	if !exists {
		return nil, ErrKeyNotFound // Assuming there's such an error
	}
	return value, nil
}

func (m *MockKvStore) Put(k string, v string) error {
	m.mu.Lock()
	defer m.mu.Unlock()
	m.data[k] = []byte(v)
	return nil
}

func (m *MockKvStore) Get(k string) (string, error) {
	m.mu.RLock()
	defer m.mu.RUnlock()
	value, exists := m.data[k]
	if !exists {
		return "", ErrKeyNotFound
	}
	return string(value), nil
}

func (m *MockKvStore) Delete(k string) error {
	m.mu.Lock()
	defer m.mu.Unlock()
	delete(m.data, k)
	return nil
}

func (m *MockKvStore) DumpPrefixKey(prefix string, trimPrefix bool) (map[string]string, error) {
	m.mu.RLock()
	defer m.mu.RUnlock()
	result := make(map[string]string)
	for k, v := range m.data {
		if len(k) >= len(prefix) && k[:len(prefix)] == prefix {
			if trimPrefix {
				result[k[len(prefix):]] = string(v)
			} else {
				result[k] = string(v)
			}
		}
	}
	return result, nil
}

func (m *MockKvStore) FlushDB() {
	// Not implemented for mock
}

func (m *MockKvStore) SeekPrefixLast(prefix []byte) ([]byte, []byte, error) {
	m.mu.RLock()
	defer m.mu.RUnlock()
	var lastKey []byte
	var lastValue []byte
	for k, v := range m.data {
		if len(k) >= len(prefix) && k[:len(prefix)] == string(prefix) {
			if lastKey == nil || k > string(lastKey) {
				lastKey = []byte(k)
				lastValue = v
			}
		}
	}
	if lastKey == nil {
		return nil, nil, ErrKeyNotFound
	}
	return lastKey, lastValue, nil
}

func (m *MockKvStore) SeekPrefixKeyIdMax(prefix []byte) (uint64, error) {
	m.mu.RLock()
	defer m.mu.RUnlock()
	var maxKeyId uint64
	for k := range m.data {
		if len(k) >= len(prefix) && k[:len(prefix)] == string(prefix) {
			// Extract the ID part after the prefix
			if len(k) > len(prefix) {
				// For simplicity, we're assuming the key format is prefix + 8-byte ID
				// In real implementation, we'd need to properly decode the key
				// For now, just return a simple counter-based approach
				// We'll handle this in the test implementation differently
			}
		}
	}
	// For this mock, we'll implement a simpler version
	return maxKeyId, nil
}

func (m *MockKvStore) SeekPrefixFirst(prefix string) ([]byte, []byte, error) {
	m.mu.RLock()
	defer m.mu.RUnlock()
	for k, v := range m.data {
		if len(k) >= len(prefix) && k[:len(prefix)] == prefix {
			return []byte(k), v, nil
		}
	}
	return nil, nil, ErrKeyNotFound
}

func (m *MockKvStore) DelPrefixKeys(prefix string) error {
	m.mu.Lock()
	defer m.mu.Unlock()
	for k := range m.data {
		if len(k) >= len(prefix) && k[:len(prefix)] == prefix {
			delete(m.data, k)
		}
	}
	return nil
}

// For tests, we need to implement a proper mock that handles the actual key encoding
type MockKvStoreWithEncoding struct {
	data map[string][]byte
	mu   sync.RWMutex
}

func NewMockKvStoreWithEncoding() *MockKvStoreWithEncoding {
	return &MockKvStoreWithEncoding{
		data: make(map[string][]byte),
	}
}

func (m *MockKvStoreWithEncoding) PutBytesKv(k []byte, v []byte) error {
	m.mu.Lock()
	defer m.mu.Unlock()
	m.data[string(k)] = v
	return nil
}

func (m *MockKvStoreWithEncoding) DeleteBytesK(k []byte) error {
	m.mu.Lock()
	defer m.mu.Unlock()
	delete(m.data, string(k))
	return nil
}

func (m *MockKvStoreWithEncoding) GetBytesValue(k []byte) ([]byte, error) {
	m.mu.RLock()
	defer m.mu.RUnlock()
	value, exists := m.data[string(k)]
	if !exists {
		return nil, assert.AnError // Using assert.AnError as a generic error
	}
	return value, nil
}

func (m *MockKvStoreWithEncoding) Put(k string, v string) error {
	m.mu.Lock()
	defer m.mu.Unlock()
	m.data[k] = []byte(v)
	return nil
}

func (m *MockKvStoreWithEncoding) Get(k string) (string, error) {
	m.mu.RLock()
	defer m.mu.RUnlock()
	value, exists := m.data[k]
	if !exists {
		return "", assert.AnError
	}
	return string(value), nil
}

func (m *MockKvStoreWithEncoding) Delete(k string) error {
	m.mu.Lock()
	defer m.mu.Unlock()
	delete(m.data, k)
	return nil
}

func (m *MockKvStoreWithEncoding) DumpPrefixKey(prefix string, trimPrefix bool) (map[string]string, error) {
	m.mu.RLock()
	defer m.mu.RUnlock()
	result := make(map[string]string)
	for k, v := range m.data {
		if len(k) >= len(prefix) && k[:len(prefix)] == prefix {
			if trimPrefix {
				result[k[len(prefix):]] = string(v)
			} else {
				result[k] = string(v)
			}
		}
	}
	return result, nil
}

func (m *MockKvStoreWithEncoding) FlushDB() {
	// Not implemented for mock
}

func (m *MockKvStoreWithEncoding) SeekPrefixLast(prefix []byte) ([]byte, []byte, error) {
	m.mu.RLock()
	defer m.mu.RUnlock()
	var lastKey []byte
	var lastValue []byte
	var found bool

	for k, v := range m.data {
		if len(k) >= len(prefix) && bytes.Equal([]byte(k)[:len(prefix)], prefix) {
			// For encoded keys, we need to check if this is the "last" key
			// In our test, we'll just pick one that matches
			if !found || k > string(lastKey) {
				lastKey = []byte(k)
				lastValue = v
				found = true
			}
		}
	}

	if !found {
		return nil, nil, assert.AnError
	}
	return lastKey, lastValue, nil
}

func (m *MockKvStoreWithEncoding) SeekPrefixKeyIdMax(prefix []byte) (uint64, error) {
	m.mu.RLock()
	defer m.mu.RUnlock()
	var maxKeyId uint64

	for k := range m.data {
		if len(k) >= len(prefix) && bytes.Equal([]byte(k)[:len(prefix)], prefix) {
			// Extract ID from the key (assuming last 8 bytes contain the ID)
			if len(k) >= len(prefix)+8 {
				// Decode the key to extract the ID
				keyBytes := []byte(k)
				id := DecodeRaftLogKey(keyBytes)
				if id > maxKeyId {
					maxKeyId = id
				}
			}
		}
	}

	// If no keys match, return 0
	return maxKeyId, nil
}

func (m *MockKvStoreWithEncoding) SeekPrefixFirst(prefix string) ([]byte, []byte, error) {
	m.mu.RLock()
	defer m.mu.RUnlock()

	for k, v := range m.data {
		if len(k) >= len(prefix) && k[:len(prefix)] == prefix {
			return []byte(k), v, nil
		}
	}
	return nil, nil, assert.AnError
}

func (m *MockKvStoreWithEncoding) DelPrefixKeys(prefix string) error {
	m.mu.Lock()
	defer m.mu.Unlock()
	for k := range m.data {
		if len(k) >= len(prefix) && k[:len(prefix)] == prefix {
			delete(m.data, k)
		}
	}
	return nil
}

func TestRaftPersistentState(t *testing.T) {
	t.Run("should initialize RaftPersistentState struct", func(t *testing.T) {
		state := &RaftPersistentState{
			CurTerm:    1,
			VotedFor:   2,
			AppliedIdx: 3,
		}

		assert.Equal(t, int64(1), state.CurTerm)
		assert.Equal(t, int64(2), state.VotedFor)
		assert.Equal(t, int64(3), state.AppliedIdx)
	})
}

func TestMakePersistRaftLog(t *testing.T) {
	t.Run("should create RaftLog with initial entry when no entries exist", func(t *testing.T) {
		mockStore := NewMockKvStoreWithEncoding()

		raftLog := MakePersistRaftLog(mockStore)

		assert.NotNil(t, raftLog)
		assert.Equal(t, mockStore, raftLog.dbEng)

		// Verify that an initial entry was created
		initialKey := EncodeRaftLogKey(InitLogIndex) // Assuming InitLogIndex is defined somewhere
		_, err := mockStore.GetBytesValue(initialKey)
		assert.NoError(t, err)
	})

	t.Run("should create RaftLog without adding initial entry if entries exist", func(t *testing.T) {
		mockStore := NewMockKvStoreWithEncoding()
		// Add an initial entry manually
		initialKey := EncodeRaftLogKey(1)
		initialEntry := &pb.Entry{Term: 1, Index: 1, Data: []byte("initial")}
		initialEncoded := EncodeEntry(initialEntry)
		mockStore.PutBytesKv(initialKey, initialEncoded)

		raftLog := MakePersistRaftLog(mockStore)

		assert.NotNil(t, raftLog)
		assert.Equal(t, mockStore, raftLog.dbEng)
	})
}

func TestPersistRaftState_ReadRaftState(t *testing.T) {
	t.Run("should persist and read raft state", func(t *testing.T) {
		mockStore := NewMockKvStoreWithEncoding()
		raftLog := &RaftLog{dbEng: mockStore}

		curTerm := int64(5)
		votedFor := int64(10)
		appliedIdx := int64(15)

		raftLog.PersistRaftState(curTerm, votedFor, appliedIdx)

		readCurTerm, readVotedFor, readAppliedIdx := raftLog.ReadRaftState()

		assert.Equal(t, curTerm, readCurTerm)
		assert.Equal(t, votedFor, readVotedFor)
		assert.Equal(t, appliedIdx, readAppliedIdx)
	})

	t.Run("should return default values when no state exists", func(t *testing.T) {
		mockStore := NewMockKvStoreWithEncoding()
		raftLog := &RaftLog{dbEng: mockStore}

		curTerm, votedFor, appliedIdx := raftLog.ReadRaftState()

		assert.Equal(t, int64(0), curTerm)
		assert.Equal(t, int64(-1), votedFor)
		assert.Equal(t, int64(-1), appliedIdx)
	})
}

func TestPersisSnapshot_ReadSnapshot(t *testing.T) {
	t.Run("should persist and read snapshot", func(t *testing.T) {
		mockStore := NewMockKvStoreWithEncoding()
		raftLog := &RaftLog{dbEng: mockStore}

		snapshotData := []byte("snapshot context data")

		raftLog.PersisSnapshot(snapshotData)

		readSnapshot, err := raftLog.ReadSnapshot()

		assert.NoError(t, err)
		assert.Equal(t, snapshotData, readSnapshot)
	})

	t.Run("should return error when no snapshot exists", func(t *testing.T) {
		mockStore := NewMockKvStoreWithEncoding()
		raftLog := &RaftLog{dbEng: mockStore}

		_, err := raftLog.ReadSnapshot()

		assert.Error(t, err)
	})
}

func TestGetFirstLogId(t *testing.T) {
	t.Run("should return first log ID", func(t *testing.T) {
		mockStore := NewMockKvStoreWithEncoding()
		raftLog := &RaftLog{dbEng: mockStore}

		// Add some entries
		key1 := EncodeRaftLogKey(5) // This will be the first since we're adding it first
		entry1 := &pb.Entry{Term: 1, Index: 5}
		mockStore.PutBytesKv(key1, EncodeEntry(entry1))

		key2 := EncodeRaftLogKey(10)
		entry2 := &pb.Entry{Term: 2, Index: 10}
		mockStore.PutBytesKv(key2, EncodeEntry(entry2))

		firstId := raftLog.GetFirstLogId()

		assert.Equal(t, uint64(5), firstId)
	})
}

func TestGetLastLogId(t *testing.T) {
	t.Run("should return last log ID", func(t *testing.T) {
		mockStore := NewMockKvStoreWithEncoding()
		raftLog := &RaftLog{dbEng: mockStore}

		// Add some entries
		key1 := EncodeRaftLogKey(5)
		entry1 := &pb.Entry{Term: 1, Index: 5}
		mockStore.PutBytesKv(key1, EncodeEntry(entry1))

		key2 := EncodeRaftLogKey(10) // This should be the last
		entry2 := &pb.Entry{Term: 2, Index: 10}
		mockStore.PutBytesKv(key2, EncodeEntry(entry2))

		lastId := raftLog.GetLastLogId()

		assert.Equal(t, uint64(10), lastId)
	})
}

func TestSetEntFirstData(t *testing.T) {
	t.Run("should update the data of the first entry", func(t *testing.T) {
		mockStore := NewMockKvStoreWithEncoding()
		raftLog := &RaftLog{dbEng: mockStore}

		// Add an initial entry
		initialKey := EncodeRaftLogKey(1)
		initialEntry := &pb.Entry{Term: 1, Index: 1, Data: []byte("old_data")}
		mockStore.PutBytesKv(initialKey, EncodeEntry(initialEntry))

		newData := []byte("new_data")
		err := raftLog.SetEntFirstData(newData)

		assert.NoError(t, err)

		// Check that the entry was updated
		updatedValue, err := mockStore.GetBytesValue(initialKey)
		assert.NoError(t, err)

		decodedEntry := DecodeEntry(updatedValue)
		assert.Equal(t, newData, decodedEntry.Data)
		assert.Equal(t, int64(1), decodedEntry.Index) // Index should remain the same
	})
}

func TestReInitLogs(t *testing.T) {
	t.Run("should reinitialize logs to initial state", func(t *testing.T) {
		mockStore := NewMockKvStoreWithEncoding()
		raftLog := &RaftLog{dbEng: mockStore}

		// Add some entries
		key1 := EncodeRaftLogKey(5)
		entry1 := &pb.Entry{Term: 1, Index: 5}
		mockStore.PutBytesKv(key1, EncodeEntry(entry1))

		key2 := EncodeRaftLogKey(10)
		entry2 := &pb.Entry{Term: 2, Index: 10}
		mockStore.PutBytesKv(key2, EncodeEntry(entry2))

		err := raftLog.ReInitLogs()
		assert.NoError(t, err)

		// Check that old entries are gone and only initial entry remains
		_, err = mockStore.GetBytesValue(EncodeRaftLogKey(5))
		assert.Error(t, err)

		_, err = mockStore.GetBytesValue(EncodeRaftLogKey(10))
		assert.Error(t, err)

		// Check that initial entry exists
		_, err = mockStore.GetBytesValue(EncodeRaftLogKey(InitLogIndex)) // Assuming InitLogIndex exists
		assert.NoError(t, err)
	})
}

func TestSetEntFirstTermAndIndex(t *testing.T) {
	t.Run("should update the term and index of the first entry", func(t *testing.T) {
		mockStore := NewMockKvStoreWithEncoding()
		raftLog := &RaftLog{dbEng: mockStore}

		// Add an initial entry
		initialKey := EncodeRaftLogKey(1)
		initialEntry := &pb.Entry{Term: 1, Index: 1, Data: []byte("data")}
		mockStore.PutBytesKv(initialKey, EncodeEntry(initialEntry))

		newTerm := int64(5)
		newIndex := int64(10)
		err := raftLog.SetEntFirstTermAndIndex(newTerm, newIndex)

		assert.NoError(t, err)

		// Check that the entry was updated with new key
		newKey := EncodeRaftLogKey(uint64(newIndex))
		updatedValue, err := mockStore.GetBytesValue(newKey)
		assert.NoError(t, err)

		decodedEntry := DecodeEntry(updatedValue)
		assert.Equal(t, uint64(newTerm), decodedEntry.Term)
		assert.Equal(t, newIndex, decodedEntry.Index)
		assert.Equal(t, []byte("data"), decodedEntry.Data) // Data should remain the same

		// Original key should no longer exist
		_, err = mockStore.GetBytesValue(initialKey)
		assert.Error(t, err)
	})
}

func TestGetFirst(t *testing.T) {
	t.Run("should return the first entry", func(t *testing.T) {
		mockStore := NewMockKvStoreWithEncoding()
		raftLog := &RaftLog{dbEng: mockStore}

		// Add entries
		key1 := EncodeRaftLogKey(5)
		entry1 := &pb.Entry{Term: 1, Index: 5, Data: []byte("first_entry")}
		mockStore.PutBytesKv(key1, EncodeEntry(entry1))

		key2 := EncodeRaftLogKey(10)
		entry2 := &pb.Entry{Term: 2, Index: 10, Data: []byte("second_entry")}
		mockStore.PutBytesKv(key2, EncodeEntry(entry2))

		firstEntry := raftLog.GetFirst()

		assert.Equal(t, entry1.Term, firstEntry.Term)
		assert.Equal(t, entry1.Index, firstEntry.Index)
		assert.Equal(t, entry1.Data, firstEntry.Data)
	})
}

// func TestGetLast(t *testing.T) {
// 	t.Run("should return the last entry", func(t *testing.T) {
// 		mockStore := NewMockKvStoreWithEncoding()
// 		raftLog := &RaftLog{dbEng: mockStore}

// 		// Add entries
// 		key1 := EncodeRaftLogKey(5)
// 		entry1 := &pb.Entry{Term: 1, Index: 5, Data: []byte("first_entry")}
// 		mockStore.PutBytesKv(key1, EncodeEntry(entry1))

// 		key2 := EncodeRaftLogKey(10)
// 		entry2 := &pb.Entry{Term: 2, Index: 10, Data: []byte("last_entry")}
// 		mockStore.PutBytesKv(key2, EncodeEntry(entry2))

// 		lastEntry := raftLog.GetLast()

// 		assert.Equal(t, entry2.Term, lastEntry.Term)
// 		assert.Equal(t, entry2.Index, lastEntry.Index)
// 		assert.Equal(t, entry2.Data, lastEntry.Data)
// 	})
// }

func TestLogItemCount(t *testing.T) {
	t.Run("should return correct log item count", func(t *testing.T) {
		mockStore := NewMockKvStoreWithEncoding()
		raftLog := &RaftLog{dbEng: mockStore}

		// Add entries starting from index 5
		for i := uint64(5); i <= 10; i++ {
			key := EncodeRaftLogKey(i)
			entry := &pb.Entry{Term: uint64(i), Index: int64(i)}
			mockStore.PutBytesKv(key, EncodeEntry(entry))
		}

		count := raftLog.LogItemCount()

		assert.Equal(t, 6, count) // From index 5 to 10 (inclusive)
	})
}

func TestAppend(t *testing.T) {
	t.Run("should append new entry to logs", func(t *testing.T) {
		mockStore := NewMockKvStoreWithEncoding()
		raftLog := &RaftLog{dbEng: mockStore}

		// Add an initial entry
		initialKey := EncodeRaftLogKey(1)
		initialEntry := &pb.Entry{Term: 1, Index: 1}
		mockStore.PutBytesKv(initialKey, EncodeEntry(initialEntry))

		// Append a new entry
		newEntry := &pb.Entry{Term: 2, Index: 2, Data: []byte("new_data")}
		raftLog.Append(newEntry)

		// Check that the new entry was added with the next available ID
		appendedKey := EncodeRaftLogKey(2) // Should be the next ID after the max
		appendedValue, err := mockStore.GetBytesValue(appendedKey)
		assert.NoError(t, err)

		decodedEntry := DecodeEntry(appendedValue)
		assert.Equal(t, newEntry.Term, decodedEntry.Term)
		assert.Equal(t, newEntry.Data, decodedEntry.Data)
	})
}

// func TestEraseBefore(t *testing.T) {
// 	t.Run("should return entries after specified index without deleting them", func(t *testing.T) {
// 		mockStore := NewMockKvStoreWithEncoding()
// 		raftLog := &RaftLog{dbEng: mockStore}

// 		// Add entries starting from index 1
// 		for i := uint64(1); i <= 5; i++ {
// 			key := EncodeRaftLogKey(i)
// 			entry := &pb.Entry{Term: uint64(i), Index: int64(i), Data: []byte{byte(i)}}
// 			mockStore.PutBytesKv(key, EncodeEntry(entry))
// 		}

// 		// Erase before index 3 (should return entries from index 3 onwards)
// 		remainingEntries := raftLog.EraseBefore(3)

// 		assert.Len(t, remainingEntries, 2)                   // Entries at indices 4 and 5 (offsets 3 and 4)
// 		assert.Equal(t, int64(4), remainingEntries[0].Index) // Index = firstIdx + offset = 1 + 3
// 		assert.Equal(t, int64(5), remainingEntries[1].Index) // Index = firstIdx + offset = 1 + 4
// 	})
// }

func TestEraseBeforeWithDel(t *testing.T) {
	t.Run("should delete entries before specified index", func(t *testing.T) {
		mockStore := NewMockKvStoreWithEncoding()
		raftLog := &RaftLog{dbEng: mockStore}

		// Add entries starting from index 1
		for i := uint64(1); i <= 5; i++ {
			key := EncodeRaftLogKey(i)
			entry := &pb.Entry{Term: uint64(i), Index: int64(i)}
			mockStore.PutBytesKv(key, EncodeEntry(entry))
		}

		// Delete entries before index 3 (should delete entries at indices 1 and 2)
		err := raftLog.EraseBeforeWithDel(3)
		assert.NoError(t, err)

		// Check that entries 1 and 2 were deleted
		_, err = mockStore.GetBytesValue(EncodeRaftLogKey(1))
		assert.Error(t, err)
		_, err = mockStore.GetBytesValue(EncodeRaftLogKey(2))
		assert.Error(t, err)

		// Check that entries 3, 4, and 5 still exist
		_, err = mockStore.GetBytesValue(EncodeRaftLogKey(3))
		assert.NoError(t, err)
		_, err = mockStore.GetBytesValue(EncodeRaftLogKey(4))
		assert.NoError(t, err)
		_, err = mockStore.GetBytesValue(EncodeRaftLogKey(5))
		assert.NoError(t, err)
	})
}

// func TestEraseAfter(t *testing.T) {
// 	t.Run("should return entries before specified index with optional deletion", func(t *testing.T) {
// 		mockStore := NewMockKvStoreWithEncoding()
// 		raftLog := &RaftLog{dbEng: mockStore}

// 		// Add entries starting from index 1
// 		for i := uint64(1); i <= 5; i++ {
// 			key := EncodeRaftLogKey(i)
// 			entry := &pb.Entry{Term: uint64(i), Index: int64(i), Data: []byte{byte(i)}}
// 			mockStore.PutBytesKv(key, EncodeEntry(entry))
// 		}

// 		// Get entries before index 3 (should return entries at indices 1 and 2)
// 		remainingEntries := raftLog.EraseAfter(3, false) // Don't delete

// 		assert.Len(t, remainingEntries, 3) // Entries at indices 1, 2, and 3 (offsets 0, 1, 2)
// 		assert.Equal(t, int64(1), remainingEntries[0].Index)
// 		assert.Equal(t, int64(2), remainingEntries[1].Index)
// 		assert.Equal(t, int64(3), remainingEntries[2].Index)

// 		// All entries should still exist since withDel is false
// 		for i := uint64(1); i <= 5; i++ {
// 			_, err := mockStore.GetBytesValue(EncodeRaftLogKey(i))
// 			assert.NoError(t, err)
// 		}
// 	})

// 	t.Run("should delete entries after specified index when withDel is true", func(t *testing.T) {
// 		mockStore := NewMockKvStoreWithEncoding()
// 		raftLog := &RaftLog{dbEng: mockStore}

// 		// Add entries starting from index 1
// 		for i := uint64(1); i <= 5; i++ {
// 			key := EncodeRaftLogKey(i)
// 			entry := &pb.Entry{Term: uint64(i), Index: int64(i), Data: []byte{byte(i)}}
// 			mockStore.PutBytesKv(key, EncodeEntry(entry))
// 		}

// 		// Get entries before index 3 and delete the rest (after index 3)
// 		remainingEntries := raftLog.EraseAfter(3, true) // Delete entries after index 3

// 		assert.Len(t, remainingEntries, 3) // Entries at indices 1, 2, and 3 (offsets 0, 1, 2)
// 		assert.Equal(t, int64(1), remainingEntries[0].Index)
// 		assert.Equal(t, int64(2), remainingEntries[1].Index)
// 		assert.Equal(t, int64(3), remainingEntries[2].Index)

// 		// Check that entries 4 and 5 were deleted but 1, 2, 3 remain
// 		_, err := mockStore.GetBytesValue(EncodeRaftLogKey(1))
// 		assert.NoError(t, err)
// 		_, err = mockStore.GetBytesValue(EncodeRaftLogKey(2))
// 		assert.NoError(t, err)
// 		_, err = mockStore.GetBytesValue(EncodeRaftLogKey(3))
// 		assert.NoError(t, err)
// 		_, err = mockStore.GetBytesValue(EncodeRaftLogKey(4))
// 		assert.Error(t, err) // Should be deleted
// 		_, err = mockStore.GetBytesValue(EncodeRaftLogKey(5))
// 		assert.Error(t, err) // Should be deleted
// 	})
// }

// func TestGetRange(t *testing.T) {
// 	t.Run("should return entries in the specified range [lo, hi)", func(t *testing.T) {
// 		mockStore := NewMockKvStoreWithEncoding()
// 		raftLog := &RaftLog{dbEng: mockStore}

// 		// Add entries starting from index 0
// 		for i := uint64(0); i < 5; i++ {
// 			key := EncodeRaftLogKey(i)
// 			entry := &pb.Entry{Term: uint64(i), Index: int64(i), Data: []byte{byte(i)}}
// 			mockStore.PutBytesKv(key, EncodeEntry(entry))
// 		}

// 		// Get entries in range [1, 4) - should include indices 1, 2, 3
// 		entries := raftLog.GetRange(1, 4)

// 		assert.Len(t, entries, 3)
// 		assert.Equal(t, int64(1), entries[0].Index)
// 		assert.Equal(t, int64(2), entries[1].Index)
// 		assert.Equal(t, int64(3), entries[2].Index)
// 	})
// }

// func TestGetEntry(t *testing.T) {
// 	t.Run("should return entry at specified index", func(t *testing.T) {
// 		mockStore := NewMockKvStoreWithEncoding()
// 		raftLog := &RaftLog{dbEng: mockStore}

// 		// Add an entry
// 		key := EncodeRaftLogKey(2)
// 		entry := &pb.Entry{Term: 5, Index: 2, Data: []byte("test_data")}
// 		mockStore.PutBytesKv(key, EncodeEntry(entry))

// 		// Get the entry at index 2 (offset 2 from first index 0)
// 		retrievedEntry := raftLog.GetEntry(2)

// 		assert.Equal(t, entry.Term, retrievedEntry.Term)
// 		assert.Equal(t, entry.Index, retrievedEntry.Index)
// 		assert.Equal(t, entry.Data, retrievedEntry.Data)
// 	})
// }

func TestGetEnt(t *testing.T) {
	t.Run("should return entry at specified offset", func(t *testing.T) {
		mockStore := NewMockKvStoreWithEncoding()
		raftLog := &RaftLog{dbEng: mockStore}

		// Add entries starting from index 1
		for i := uint64(1); i <= 3; i++ {
			key := EncodeRaftLogKey(i)
			entry := &pb.Entry{Term: uint64(i), Index: int64(i), Data: []byte{byte(i)}}
			mockStore.PutBytesKv(key, EncodeEntry(entry))
		}

		// Get entry at offset 1 (which should be the entry at index 2)
		retrievedEntry := raftLog.GetEnt(1)

		assert.Equal(t, int64(2), retrievedEntry.Index) // First index is 1, offset 1 means index 1+1=2
		assert.Equal(t, int64(2), retrievedEntry.Term)
		assert.Equal(t, []byte{2}, retrievedEntry.Data)
	})
}

func TestEncodeDecodeRaftLogKey(t *testing.T) {
	t.Run("should encode and decode raft log keys correctly", func(t *testing.T) {
		testIdx := uint64(12345)

		encodedKey := EncodeRaftLogKey(testIdx)
		decodedIdx := DecodeRaftLogKey(encodedKey)

		assert.Equal(t, testIdx, decodedIdx)
	})
}

func TestEncodeDecodeEntry(t *testing.T) {
	t.Run("should encode and decode entries correctly", func(t *testing.T) {
		originalEntry := &pb.Entry{
			Term:  5,
			Index: 10,
			Data:  []byte("test data"),
		}

		encoded := EncodeEntry(originalEntry)
		decoded := DecodeEntry(encoded)

		assert.Equal(t, originalEntry.Term, decoded.Term)
		assert.Equal(t, originalEntry.Index, decoded.Index)
		assert.Equal(t, originalEntry.Data, decoded.Data)
	})
}

func TestEncodeDecodeRaftState(t *testing.T) {
	t.Run("should encode and decode raft state correctly", func(t *testing.T) {
		originalState := &RaftPersistentState{
			CurTerm:    5,
			VotedFor:   10,
			AppliedIdx: 15,
		}

		encoded := EncodeRaftState(originalState)
		decoded := DecodeRaftState(encoded)

		assert.Equal(t, originalState.CurTerm, decoded.CurTerm)
		assert.Equal(t, originalState.VotedFor, decoded.VotedFor)
		assert.Equal(t, originalState.AppliedIdx, decoded.AppliedIdx)
	})
}
