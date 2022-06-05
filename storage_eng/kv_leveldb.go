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

package storage_eng

import (
	"encoding/binary"
	"errors"

	"github.com/syndtr/goleveldb/leveldb"
	"github.com/syndtr/goleveldb/leveldb/opt"
	"github.com/syndtr/goleveldb/leveldb/util"
)

type LevelDBKvStore struct {
	Path string
	db   *leveldb.DB
}

func MakeLevelDBKvStore(path string) (*LevelDBKvStore, error) {
	newDB, err := leveldb.OpenFile(path, &opt.Options{})
	if err != nil {
		return nil, err
	}
	return &LevelDBKvStore{
		Path: path,
		db:   newDB,
	}, nil
}

func (levelDB *LevelDBKvStore) PutBytesKv(k []byte, v []byte) error {
	return levelDB.db.Put(k, v, nil)
}

func (levelDB *LevelDBKvStore) DeleteBytesK(k []byte) error {
	return levelDB.db.Delete(k, nil)
}

func (levelDB *LevelDBKvStore) GetBytesValue(k []byte) ([]byte, error) {
	return levelDB.db.Get(k, nil)
}

func (levelDB *LevelDBKvStore) Put(k string, v string) error {
	return levelDB.db.Put([]byte(k), []byte(v), nil)
}

func (levelDB *LevelDBKvStore) Get(k string) (string, error) {
	v, err := levelDB.db.Get([]byte(k), nil)
	if err != nil {
		return "", err
	}
	return string(v), nil
}

func (levelDB *LevelDBKvStore) Delete(k string) error {
	return levelDB.db.Delete([]byte(k), nil)
}

func (levelDB *LevelDBKvStore) DumpPrefixKey(prefix string) (map[string]string, error) {
	kvs := make(map[string]string)
	iter := levelDB.db.NewIterator(util.BytesPrefix([]byte(prefix)), nil)
	for iter.Next() {
		k := string(iter.Key())
		v := string(iter.Value())
		kvs[k] = v
	}
	iter.Release()
	return kvs, iter.Error()
}

func (levelDB *LevelDBKvStore) FlushDB() {

}

func (levelDB *LevelDBKvStore) SeekPrefixLast(prefix []byte) ([]byte, []byte, error) {
	iter := levelDB.db.NewIterator(util.BytesPrefix(prefix), nil)
	defer iter.Release()
	ok := iter.Last()
	var keyBytes, valBytes []byte
	if ok {
		keyBytes = iter.Key()
		valBytes = iter.Value()
	}
	return keyBytes, valBytes, nil
}

func (levelDB *LevelDBKvStore) SeekPrefixKeyIdMax(prefix []byte) (uint64, error) {
	iter := levelDB.db.NewIterator(util.BytesPrefix(prefix), nil)
	defer iter.Release()
	var maxKeyId uint64
	maxKeyId = 0
	for iter.Next() {
		if iter.Error() != nil {
			return maxKeyId, iter.Error()
		}
		kBytes := iter.Key()
		KeyId := binary.LittleEndian.Uint64(kBytes[len(prefix):])
		if KeyId > maxKeyId {
			maxKeyId = KeyId
		}
	}
	return maxKeyId, nil
}

func (levelDB *LevelDBKvStore) SeekPrefixFirst(prefix string) ([]byte, []byte, error) {
	iter := levelDB.db.NewIterator(util.BytesPrefix([]byte(prefix)), nil)
	defer iter.Release()
	if iter.Next() {
		return iter.Key(), iter.Value(), nil
	}
	return []byte{}, []byte{}, errors.New("seek not find key")
}

func (levelDB *LevelDBKvStore) DelPrefixKeys(prefix string) error {
	iter := levelDB.db.NewIterator(util.BytesPrefix([]byte(prefix)), nil)
	for iter.Next() {
		err := levelDB.db.Delete(iter.Key(), nil)
		if err != nil {
			return err
		}
	}
	iter.Release()
	return nil
}
