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
	"github.com/eraft-io/eraft/pkg/log"
	"github.com/syndtr/goleveldb/leveldb"
	"github.com/syndtr/goleveldb/leveldb/opt"
	"github.com/syndtr/goleveldb/leveldb/util"
)

type KvStoreLevelDB struct {
	path string
	db   *leveldb.DB
}

func MakeLevelDBKvStore(dbPath string) (*KvStoreLevelDB, error) {
	leveldb, err := leveldb.OpenFile(dbPath, &opt.Options{})
	if err != nil {
		return nil, err
	}
	return &KvStoreLevelDB{
		path: dbPath,
		db:   leveldb,
	}, nil
}

func (ldb *KvStoreLevelDB) Put(k []byte, v []byte) error {
	return ldb.db.Put(k, v, nil)
}

func (ldb *KvStoreLevelDB) Get(k []byte) ([]byte, error) {
	return ldb.db.Get(k, nil)
}

func (ldb *KvStoreLevelDB) Del(k []byte) error {
	return ldb.db.Delete(k, nil)
}

func (ldb *KvStoreLevelDB) GetPrefixRangeKvs(prefix []byte) ([]string, []string, error) {
	keys := make([]string, 0)
	vals := make([]string, 0)
	iter := ldb.db.NewIterator(util.BytesPrefix([]byte(prefix)), nil)
	for iter.Next() {
		log.MainLogger.Debug().Msgf("leveldb iter key -> %v, val -> %v", iter.Key(), iter.Value())
		keys = append(keys, string(iter.Key()))
		vals = append(vals, string(iter.Value()))
	}
	iter.Release()
	return keys, vals, nil
}
