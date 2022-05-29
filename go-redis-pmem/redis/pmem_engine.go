///////////////////////////////////////////////////////////////////////
// Copyright 2018-2019 VMware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////
package redis

import (
	"errors"
	// "bytes"
	// "encoding/binary"
	// "fmt"
	"github.com/vmware/go-pmem-transaction/pmem"
	"github.com/vmware/go-pmem-transaction/transaction"
)

type PMemKvStore struct {
	dbr    *redisDb
	dbPath string
}

func MakePMemKvStore(path string) (*PMemKvStore, error) {
	firstInit := pmem.Init(path)
	var dbr *redisDb

	if firstInit { // indicates a first time initialization
		db := (*redisDb)(pmem.New("dbRoot", dbr))
		populateDb(db)
		dbr = db
	} else {
		db := (*redisDb)(pmem.Get("dbRoot", dbr))
		if db.magic != MAGIC {
			// Previous initialization did not complete successfully. Re-populate
			// data members in db.
			populateDb(db)
		}
		txn("undo") {
			db.swizzle()
		}
		dbr = db
	}

	return &PMemKvStore{
		dbr:    dbr,
		dbPath: path,
	}, nil
}

func (pmemSt *PMemKvStore) Put(k string, v string) error {
	isOk := pmemSt.dbr.setKey(shadowCopyToPmem([]byte(k)), shadowCopyToPmemI([]byte(v)))
	if !isOk {
		return errors.New("set key not ok!")
	}
	return nil
}

func B2S(bs []uint8) string {
	ba := []byte{}
	for _, b := range bs {
		ba = append(ba, byte(b))
	}
	return string(ba)
}

func (pmemSt *PMemKvStore) Get(k string) (string, error) {
	val := pmemSt.dbr.lookupKey([]byte(k))
	valBytes := val.(*[]uint8)
	return B2S(*valBytes), nil
}

func (pmemSt *PMemKvStore) Delete(k string) error {
	isOk := pmemSt.dbr.delete(shadowCopyToPmem([]byte(k)))
	if !isOk {
		return errors.New("delete key not ok!")
	}
	return nil
}

func (pmemSt *PMemKvStore) DumpPrefixKey(string) (map[string]string, error) {
	return nil, nil
}

func (pmemSt *PMemKvStore) PutBytesKv(k []byte, v []byte) error {
	return nil
}

func (pmemSt *PMemKvStore) DeleteBytesK(k []byte) error {
	return nil
}

func (pmemSt *PMemKvStore) GetBytesValue(k []byte) ([]byte, error) {
	return make([]byte, 0), nil
}

func (pmemSt *PMemKvStore) SeekPrefixLast(prefix string) ([]byte, []byte, error) {
	return make([]byte, 0), make([]byte, 0), nil
}

func (pmemSt *PMemKvStore) SeekPrefixFirst(prefix string) ([]byte, []byte, error) {
	return make([]byte, 0), make([]byte, 0), nil
}

func (pmemSt *PMemKvStore) DelPrefixKeys(prefix string) error {
	return nil
}
