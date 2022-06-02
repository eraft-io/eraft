///////////////////////////////////////////////////////////////////////
// Copyright 2018-2019 VMware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////
package redis

type PMemDictStore struct {
	dbPtr  *server
	dbPath string
}

func MakePMemDictStore(path string) (*PMemDictStore, error) {
	var newdbPtr *server
	newdbPtr = RunAndReturnServer()
	return &PMemDictStore{
		dbPtr:  newdbPtr,
		dbPath: path,
	}, nil
}

func (pmemSt *PMemDictStore) Put(k string, v string) error {
	return pmemSt.dbPtr.SetKV(k, v)
}

func (pmemSt *PMemDictStore) Get(k string) (error, string) {
	return pmemSt.dbPtr.GetV(k)
}

func (pmemSt *PMemDictStore) Delete(k string) error {
	return pmemSt.dbPtr.DeleteK(k)
}

func (pmemSt *PMemDictStore) DumpPrefixKey(prefix string) (map[string]string, error) {

	return nil, nil
}

func (pmemSt *PMemDictStore) PutBytesKv(k []byte, v []byte) error {

	return nil
}

func (pmemSt *PMemDictStore) DeleteBytesK(k []byte) error {
	return nil
}

func (pmemSt *PMemDictStore) GetBytesValue(k []byte) ([]byte, error) {
	return []byte{}, nil
}

func (pmemSt *PMemDictStore) SeekPrefixLast(prefix string) ([]byte, []byte, error) {
	return []byte{}, []byte{}, nil
}

func (pmemSt *PMemDictStore) SeekPrefixFirst(prefix string) ([]byte, []byte, error) {
	return []byte{}, []byte{}, nil
}

func (pmemSt *PMemDictStore) DelPrefixKeys(prefix string) error {
	return nil
}
