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

package engine

import (
	"fmt"
	"testing"

	eng "github.com/eraft-io/eraft/pkg/engine"
	"github.com/stretchr/testify/assert"
	"github.com/syndtr/goleveldb/leveldb"
	"github.com/syndtr/goleveldb/leveldb/opt"
	"github.com/syndtr/goleveldb/leveldb/util"
)

func TestPutGetLeveldb(t *testing.T) {
	ldb := eng.KvStoreFactory("leveldb", "./test_data")
	keyBytes := []byte{0x11, 0x11, 0x19, 0x96}
	valBytes := []byte{0x11, 0x11, 0x19, 0x97}
	if err := ldb.Put(keyBytes, valBytes); err != nil {
		panic(err.Error())
	}
	gotBytes, err := ldb.Get(keyBytes)
	if err != nil {
		panic(err)
	}
	assert.Equal(t, valBytes, gotBytes)
	RemoveDir("./test_data")
}

func TestSnapshotLeveldb(t *testing.T) {
	leveldb, err := leveldb.OpenFile("./test_data", &opt.Options{})
	if err != nil {
		panic(err)
	}
	leveldb.Put([]byte{0x11, 0x12}, []byte{0x32, 0x34}, nil)
	leveldb.Put([]byte{0x14, 0x12}, []byte{0x52, 0x34}, nil)
	leveldb.Put([]byte{0x21, 0x12}, []byte{0x36, 0x34}, nil)
	snapshot, err := leveldb.GetSnapshot()
	if err != nil {
		panic(err)
	}
	defer snapshot.Release()
	b, err := snapshot.Get([]byte{0x11, 0x12}, nil)
	if err != nil {
		panic(err)
	}
	assert.Equal(t, b, []byte{0x32, 0x34})
	t.Log(fmt.Print(snapshot.String()))
	RemoveDir("./test_data")
}

func TestIterLeveldb(t *testing.T) {
	leveldb, err := leveldb.OpenFile("./test_data", &opt.Options{})
	if err != nil {
		panic(err)
	}
	leveldb.Put([]byte{0x11, 0x12}, []byte{0x32, 0x34}, nil)
	leveldb.Put([]byte{0x11, 0x13}, []byte{0x52, 0x34}, nil)
	leveldb.Put([]byte{0x11, 0x14}, []byte{0x36, 0x34}, nil)

	iter := leveldb.NewIterator(util.BytesPrefix([]byte{0x11}), &opt.ReadOptions{})
	defer iter.Release()
	i := int32(0)
	for iter.Next() {
		t.Log(fmt.Println(string(iter.Key()) + string(iter.Value())))
		i++
	}
	assert.Equal(t, i, int32(3))
	RemoveDir("./test_data")
}
