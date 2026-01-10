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
package storage

import (
	"encoding/binary"
	"path/filepath"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestMakeLevelDBKvStore(t *testing.T) {
	t.Run("should create LevelDB store successfully with valid path", func(t *testing.T) {
		tempDir := t.TempDir()
		dbPath := filepath.Join(tempDir, "test_db")

		store, err := MakeLevelDBKvStore(dbPath)

		require.NoError(t, err)
		assert.NotNil(t, store)
		assert.Equal(t, dbPath, store.Path)
		assert.NotNil(t, store.db)

		// Clean up
		if store != nil && store.db != nil {
			err = store.db.Close()
			assert.NoError(t, err)
		}
	})

	t.Run("should return error for invalid path", func(t *testing.T) {
		invalidPath := "/invalid/path/that/does/not/exist/db"

		store, err := MakeLevelDBKvStore(invalidPath)

		assert.Error(t, err)
		assert.Nil(t, store)
	})
}

func TestLevelDBKvStore_PutBytesKv_GetBytesValue(t *testing.T) {
	tempDir := t.TempDir()
	dbPath := filepath.Join(tempDir, "test_db")
	store, err := MakeLevelDBKvStore(dbPath)
	require.NoError(t, err)
	defer store.db.Close()

	t.Run("should put and get byte values", func(t *testing.T) {
		key := []byte("test_key_bytes")
		value := []byte("test_value_bytes")

		err := store.PutBytesKv(key, value)
		assert.NoError(t, err)

		retrievedValue, err := store.GetBytesValue(key)
		assert.NoError(t, err)
		assert.Equal(t, value, retrievedValue)
	})

	t.Run("should return error when key does not exist", func(t *testing.T) {
		nonExistentKey := []byte("non_existent_key")

		_, err := store.GetBytesValue(nonExistentKey)
		assert.Error(t, err)
	})
}

func TestLevelDBKvStore_DeleteBytesK(t *testing.T) {
	tempDir := t.TempDir()
	dbPath := filepath.Join(tempDir, "test_db")
	store, err := MakeLevelDBKvStore(dbPath)
	require.NoError(t, err)
	defer store.db.Close()

	t.Run("should delete byte key successfully", func(t *testing.T) {
		key := []byte("delete_test_key")
		value := []byte("delete_test_value")

		err := store.PutBytesKv(key, value)
		assert.NoError(t, err)

		// Verify the key exists
		_, err = store.GetBytesValue(key)
		assert.NoError(t, err)

		// Delete the key
		err = store.DeleteBytesK(key)
		assert.NoError(t, err)

		// Verify the key no longer exists
		_, err = store.GetBytesValue(key)
		assert.Error(t, err)
	})
}

func TestLevelDBKvStore_Put_Get_Delete(t *testing.T) {
	tempDir := t.TempDir()
	dbPath := filepath.Join(tempDir, "test_db")
	store, err := MakeLevelDBKvStore(dbPath)
	require.NoError(t, err)
	defer store.db.Close()

	t.Run("should put and get string values", func(t *testing.T) {
		key := "test_key"
		value := "test_value"

		err := store.Put(key, value)
		assert.NoError(t, err)

		retrievedValue, err := store.Get(key)
		assert.NoError(t, err)
		assert.Equal(t, value, retrievedValue)
	})

	t.Run("should delete string key successfully", func(t *testing.T) {
		key := "delete_test_key"
		value := "delete_test_value"

		err := store.Put(key, value)
		assert.NoError(t, err)

		// Verify the key exists
		_, err = store.Get(key)
		assert.NoError(t, err)

		// Delete the key
		err = store.Delete(key)
		assert.NoError(t, err)

		// Verify the key no longer exists
		_, err = store.Get(key)
		assert.Error(t, err)
	})

	t.Run("should return error when string key does not exist", func(t *testing.T) {
		nonExistentKey := "non_existent_key"

		_, err := store.Get(nonExistentKey)
		assert.Error(t, err)
	})
}

func TestLevelDBKvStore_DumpPrefixKey(t *testing.T) {
	tempDir := t.TempDir()
	dbPath := filepath.Join(tempDir, "test_db")
	store, err := MakeLevelDBKvStore(dbPath)
	require.NoError(t, err)
	defer store.db.Close()

	// Insert some test data
	testData := map[string]string{
		"prefix_a":  "value1",
		"prefix_b":  "value2",
		"prefix_c":  "value3",
		"different": "value4",
	}

	for k, v := range testData {
		err := store.Put(k, v)
		require.NoError(t, err)
	}

	t.Run("should dump all keys with prefix", func(t *testing.T) {
		result, err := store.DumpPrefixKey("prefix_", false)
		assert.NoError(t, err)
		assert.Len(t, result, 3) // Should have 3 keys matching prefix
		assert.Equal(t, "value1", result["prefix_a"])
		assert.Equal(t, "value2", result["prefix_b"])
		assert.Equal(t, "value3", result["prefix_c"])
	})

	t.Run("should dump keys with prefix and trim prefix", func(t *testing.T) {
		result, err := store.DumpPrefixKey("prefix_", true)
		assert.NoError(t, err)
		assert.Len(t, result, 3)
		assert.Equal(t, "value1", result["a"])    // prefix trimmed
		assert.Equal(t, "value2", result["b"])    // prefix trimmed
		assert.Equal(t, "value3", result["c"])    // prefix trimmed
		assert.NotContains(t, result, "prefix_a") // original key names shouldn't appear
	})

	t.Run("should return empty map for non-existent prefix", func(t *testing.T) {
		result, err := store.DumpPrefixKey("nonexistent_", false)
		assert.NoError(t, err)
		assert.Empty(t, result)
	})
}

func TestLevelDBKvStore_SeekPrefixLast(t *testing.T) {
	tempDir := t.TempDir()
	dbPath := filepath.Join(tempDir, "test_db")
	store, err := MakeLevelDBKvStore(dbPath)
	require.NoError(t, err)
	defer store.db.Close()

	prefix := []byte("test_prefix_")
	keys := [][]byte{
		append(prefix, []byte("1")...),
		append(prefix, []byte("2")...),
		append(prefix, []byte("3")...),
	}

	// Insert test data
	for _, key := range keys {
		value := []byte("value_" + string(key))
		err := store.PutBytesKv(key, value)
		require.NoError(t, err)
	}

	t.Run("should return the last key-value pair with given prefix", func(t *testing.T) {
		lastKey, lastValue, err := store.SeekPrefixLast(prefix)
		assert.NoError(t, err)
		assert.Equal(t, keys[2], lastKey) // Last key should be "test_prefix_3"
		assert.Equal(t, []byte("value_"+string(keys[2])), lastValue)
	})

	t.Run("should return empty for non-existent prefix", func(t *testing.T) {
		nonExistentPrefix := []byte("nonexistent_")
		lastKey, lastValue, err := store.SeekPrefixLast(nonExistentPrefix)
		assert.NoError(t, err)
		assert.Empty(t, lastKey)
		assert.Empty(t, lastValue)
	})
}

func TestLevelDBKvStore_SeekPrefixKeyIdMax(t *testing.T) {
	tempDir := t.TempDir()
	dbPath := filepath.Join(tempDir, "test_db")
	store, err := MakeLevelDBKvStore(dbPath)
	require.NoError(t, err)
	defer store.db.Close()

	prefix := []byte("test_prefix_")

	// Create keys with numeric IDs at the end
	key1 := append(append([]byte(nil), prefix...), make([]byte, 8)...)
	binary.LittleEndian.PutUint64(key1[len(prefix):], 5)

	key2 := append(append([]byte(nil), prefix...), make([]byte, 8)...)
	binary.LittleEndian.PutUint64(key2[len(prefix):], 7)

	key3 := append(append([]byte(nil), prefix...), make([]byte, 8)...)
	binary.LittleEndian.PutUint64(key3[len(prefix):], 3)

	// Insert test data
	for _, key := range [][][]byte{{key1, []byte("value1")}, {key2, []byte("value2")}, {key3, []byte("value3")}} {
		err := store.PutBytesKv(key[0], key[1])
		require.NoError(t, err)
	}

	t.Run("should return the maximum key ID with given prefix", func(t *testing.T) {
		maxKeyId, err := store.SeekPrefixKeyIdMax(prefix)
		assert.NoError(t, err)
		assert.Equal(t, uint64(7), maxKeyId) // Key with ID 7 should be the max
	})

	t.Run("should return 0 for non-existent prefix", func(t *testing.T) {
		nonExistentPrefix := []byte("nonexistent_")
		maxKeyId, err := store.SeekPrefixKeyIdMax(nonExistentPrefix)
		assert.NoError(t, err)
		assert.Equal(t, uint64(0), maxKeyId)
	})
}

func TestLevelDBKvStore_SeekPrefixFirst(t *testing.T) {
	tempDir := t.TempDir()
	dbPath := filepath.Join(tempDir, "test_db")
	store, err := MakeLevelDBKvStore(dbPath)
	require.NoError(t, err)
	defer store.db.Close()

	prefix := "test_prefix_"
	keys := []string{
		prefix + "1",
		prefix + "2",
		prefix + "3",
	}

	// Insert test data
	for _, key := range keys {
		value := "value_" + key
		err := store.Put(key, value)
		require.NoError(t, err)
	}

	t.Run("should return the first key-value pair with given prefix", func(t *testing.T) {
		firstKey, firstValue, err := store.SeekPrefixFirst(prefix)
		assert.NoError(t, err)
		assert.Equal(t, []byte(keys[0]), firstKey) // First key should be "test_prefix_1"
		assert.Equal(t, []byte("value_"+keys[0]), firstValue)
	})

	t.Run("should return error for non-existent prefix", func(t *testing.T) {
		_, _, err := store.SeekPrefixFirst("nonexistent_")
		assert.Error(t, err)
		assert.EqualError(t, err, "seek not find key")
	})
}

func TestLevelDBKvStore_DelPrefixKeys(t *testing.T) {
	tempDir := t.TempDir()
	dbPath := filepath.Join(tempDir, "test_db")
	store, err := MakeLevelDBKvStore(dbPath)
	require.NoError(t, err)
	defer store.db.Close()

	// Insert test data with different prefixes
	dataWithPrefix := map[string]string{
		"prefix_a": "value1",
		"prefix_b": "value2",
		"prefix_c": "value3",
	}

	dataWithoutPrefix := map[string]string{
		"different1": "value4",
		"different2": "value5",
	}

	// Insert both sets of data
	for k, v := range dataWithPrefix {
		err := store.Put(k, v)
		require.NoError(t, err)
	}
	for k, v := range dataWithoutPrefix {
		err := store.Put(k, v)
		require.NoError(t, err)
	}

	t.Run("should delete all keys with specific prefix", func(t *testing.T) {
		// Verify all prefixed keys exist before deletion
		for k := range dataWithPrefix {
			_, err := store.Get(k)
			assert.NoError(t, err)
		}

		// Delete keys with prefix
		err := store.DelPrefixKeys("prefix_")
		assert.NoError(t, err)

		// Verify that prefixed keys were deleted
		for k := range dataWithPrefix {
			_, err := store.Get(k)
			assert.Error(t, err)
		}

		// Verify that non-prefixed keys still exist
		for k, v := range dataWithoutPrefix {
			retrievedValue, err := store.Get(k)
			assert.NoError(t, err)
			assert.Equal(t, v, retrievedValue)
		}
	})
}

func TestLevelDBKvStore_FlushDB(t *testing.T) {
	tempDir := t.TempDir()
	dbPath := filepath.Join(tempDir, "test_db")
	store, err := MakeLevelDBKvStore(dbPath)
	require.NoError(t, err)
	defer store.db.Close()

	// The function currently has no implementation
	// This test ensures it doesn't panic or cause issues
	t.Run("should not panic when calling FlushDB", func(t *testing.T) {
		store.FlushDB() // Currently empty implementation
		// No assertion needed - just ensuring it doesn't panic
	})
}
