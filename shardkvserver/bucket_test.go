//
// MIT License

// Copyright (c) 2026 eraft dev group

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

package shardkvserver

import (
	"errors"
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/mock"
)

// MockKvStore 模拟存储引擎
type MockKvStore struct {
	mock.Mock
}

func (m *MockKvStore) Get(key string) (string, error) {
	args := m.Called(key)
	return args.String(0), args.Error(1)
}

func (m *MockKvStore) Put(key, value string) error {
	args := m.Called(key, value)
	return args.Error(0)
}

func (m *MockKvStore) Del(key string) error {
	args := m.Called(key)
	return args.Error(0)
}

func (m *MockKvStore) DumpPrefixKey(prefix string, trim bool) (map[string]string, error) {
	args := m.Called(prefix, trim)
	return args.Get(0).(map[string]string), args.Error(1)
}

func (m *MockKvStore) DelPrefixKeys(prefix string) error {
	args := m.Called(prefix)
	return args.Error(0)
}

// Add the missing Delete method to implement the KvStore interface
func (m *MockKvStore) Delete(key string) error {
	args := m.Called(key)
	return args.Error(0)
}

func (m *MockKvStore) DeleteBytesK(k []byte) error { return nil }

func (m *MockKvStore) PutBytesKv(k []byte, v []byte) error { return nil }

func (m *MockKvStore) FlushDB() {}

func (m *MockKvStore) GetBytesValue(k []byte) ([]byte, error) { return nil, nil }

func (m *MockKvStore) SeekPrefixFirst(prefix string) ([]byte, []byte, error) { return nil, nil, nil }

func (m *MockKvStore) SeekPrefixKeyIdMax(prefix []byte) (uint64, error) { return 0, nil }

func (m *MockKvStore) SeekPrefixLast(prefix []byte) ([]byte, []byte, error) { return nil, nil, nil }

// TestNewBucket tests the NewBucket function
func TestNewBucket(t *testing.T) {
	mockStore := new(MockKvStore)
	bucket := NewBucket(mockStore, 5)

	assert.Equal(t, 5, bucket.ID)
	assert.Equal(t, mockStore, bucket.KvDB)
	assert.Equal(t, Running, bucket.Status)
}

// TestBucketGet tests the Get method
func TestBucketGet(t *testing.T) {
	t.Run("successful get", func(t *testing.T) {
		mockStore := new(MockKvStore)
		bucket := NewBucket(mockStore, 7)

		expectedKey := "7$^$test_key"
		expectedValue := "test_value"

		mockStore.On("Get", expectedKey).Return(expectedValue, nil).Once()

		value, err := bucket.Get("test_key")
		assert.NoError(t, err)
		assert.Equal(t, expectedValue, value)

		mockStore.AssertExpectations(t)
	})

	t.Run("get with error", func(t *testing.T) {
		mockStore := new(MockKvStore)
		bucket := NewBucket(mockStore, 3)

		expectedKey := "3$^$error_key"
		testError := errors.New("storage error")

		mockStore.On("Get", expectedKey).Return("", testError).Once()

		value, err := bucket.Get("error_key")
		assert.Error(t, err)
		assert.Equal(t, testError, err)
		assert.Equal(t, "", value)

		mockStore.AssertExpectations(t)
	})
}

// TestBucketPut tests the Put method
func TestBucketPut(t *testing.T) {
	t.Run("successful put", func(t *testing.T) {
		mockStore := new(MockKvStore)
		bucket := NewBucket(mockStore, 2)

		expectedKey := "2$^$put_key"
		value := "put_value"

		mockStore.On("Put", expectedKey, value).Return(nil).Once()

		err := bucket.Put("put_key", value)
		assert.NoError(t, err)

		mockStore.AssertExpectations(t)
	})

	t.Run("put with error", func(t *testing.T) {
		mockStore := new(MockKvStore)
		bucket := NewBucket(mockStore, 4)

		expectedKey := "4$^$error_put_key"
		value := "error_value"
		testError := errors.New("put error")

		mockStore.On("Put", expectedKey, value).Return(testError).Once()

		err := bucket.Put("error_put_key", value)
		assert.Error(t, err)
		assert.Equal(t, testError, err)

		mockStore.AssertExpectations(t)
	})
}

// TestBucketAppend tests the Append method
func TestBucketAppend(t *testing.T) {
	t.Run("successful append", func(t *testing.T) {
		mockStore := new(MockKvStore)
		bucket := NewBucket(mockStore, 6)

		key := "append_key"
		expectedKey := "6$^$append_key"
		oldValue := "old_"
		newValue := "new"
		finalValue := "old_new"

		// First call for Get
		mockStore.On("Get", expectedKey).Return(oldValue, nil).Once()
		// Second call for Put
		mockStore.On("Put", expectedKey, finalValue).Return(nil).Once()

		err := bucket.Append(key, newValue)
		assert.NoError(t, err)

		mockStore.AssertExpectations(t)
	})

	t.Run("append with get error", func(t *testing.T) {
		mockStore := new(MockKvStore)
		bucket := NewBucket(mockStore, 8)

		key := "error_append_key"
		expectedKey := "8$^$error_append_key"
		testError := errors.New("get error")

		mockStore.On("Get", expectedKey).Return("", testError).Once()

		err := bucket.Append(key, "any_value")
		assert.Error(t, err)
		assert.Equal(t, testError, err)

		mockStore.AssertExpectations(t)
	})

	t.Run("append with put error", func(t *testing.T) {
		mockStore := new(MockKvStore)
		bucket := NewBucket(mockStore, 9)

		key := "put_error_append_key"
		expectedKey := "9$^$put_error_append_key"
		oldValue := "old_"
		newValue := "new"
		putError := errors.New("put error")

		// First call for Get - success
		mockStore.On("Get", expectedKey).Return(oldValue, nil).Once()
		// Second call for Put - failure
		mockStore.On("Put", expectedKey, oldValue+newValue).Return(putError).Once()

		err := bucket.Append(key, newValue)
		assert.Error(t, err)
		assert.Equal(t, putError, err)

		mockStore.AssertExpectations(t)
	})
}

// TestBucketDeepCopy tests the deepCopy method
func TestBucketDeepCopy(t *testing.T) {
	t.Run("successful deep copy with trim prefix true", func(t *testing.T) {
		mockStore := new(MockKvStore)
		bucket := NewBucket(mockStore, 10)

		expectedPrefix := "10$^$"
		trim := true
		expectedResult := map[string]string{
			"key1": "value1",
			"key2": "value2",
		}

		mockStore.On("DumpPrefixKey", expectedPrefix, trim).Return(expectedResult, nil).Once()

		result, err := bucket.deepCopy(trim)
		assert.NoError(t, err)
		assert.Equal(t, expectedResult, result)

		mockStore.AssertExpectations(t)
	})

	t.Run("successful deep copy with trim prefix false", func(t *testing.T) {
		mockStore := new(MockKvStore)
		bucket := NewBucket(mockStore, 11)

		expectedPrefix := "11$^$"
		trim := false
		expectedResult := map[string]string{
			"11$^$key1": "value1",
			"11$^$key2": "value2",
		}

		mockStore.On("DumpPrefixKey", expectedPrefix, trim).Return(expectedResult, nil).Once()

		result, err := bucket.deepCopy(trim)
		assert.NoError(t, err)
		assert.Equal(t, expectedResult, result)

		mockStore.AssertExpectations(t)
	})

	t.Run("deep copy with error", func(t *testing.T) {
		mockStore := new(MockKvStore)
		bucket := NewBucket(mockStore, 12)

		expectedPrefix := "12$^$"
		trim := true
		testError := errors.New("dump error")

		mockStore.On("DumpPrefixKey", expectedPrefix, trim).Return((map[string]string)(nil), testError).Once()

		result, err := bucket.deepCopy(trim)
		assert.Error(t, err)
		assert.Nil(t, result)
		assert.Equal(t, testError, err)

		mockStore.AssertExpectations(t)
	})
}

// TestBucketDeleteBucketData tests the deleteBucketData method
func TestBucketDeleteBucketData(t *testing.T) {
	t.Run("successful delete", func(t *testing.T) {
		mockStore := new(MockKvStore)
		bucket := NewBucket(mockStore, 13)

		expectedPrefix := "13$^$"

		mockStore.On("DelPrefixKeys", expectedPrefix).Return(nil).Once()

		err := bucket.deleteBucketData()
		assert.NoError(t, err)

		mockStore.AssertExpectations(t)
	})

	t.Run("delete with error", func(t *testing.T) {
		mockStore := new(MockKvStore)
		bucket := NewBucket(mockStore, 14)

		expectedPrefix := "14$^$"
		testError := errors.New("delete error")

		mockStore.On("DelPrefixKeys", expectedPrefix).Return(testError).Once()

		err := bucket.deleteBucketData()
		assert.Error(t, err)
		assert.Equal(t, testError, err)

		mockStore.AssertExpectations(t)
	})
}
