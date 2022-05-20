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

package shardkvserver

import (
	"strconv"

	"github.com/eraft-io/mit6.824lab2product/storage_eng"
)

type buketStatus uint8

const (
	Runing buketStatus = iota
	Stopped
)

const SPLIT = "$^$"

//
// a bucket is a logical partition in a distributed system
//  it has a unique id, a pointer to db engine, and status
//
type Bucket struct {
	ID     int
	KvDB   storage_eng.KvStore
	Status buketStatus
}

//
// this is all of a bucket datas view
// use to query a buckets data or insert data to bucket
//
type BucketDatasVo struct {
	Datas map[int]map[string]string
}

//
// make a new bucket
//
func NewBucket(eng storage_eng.KvStore, id int) *Bucket {
	return &Bucket{id, eng, Runing}
}

//
// get encode key data from engine
//
func (bu *Bucket) Get(key string) (string, error) {
	encodeKey := strconv.Itoa(bu.ID) + SPLIT + key
	v, err := bu.KvDB.Get(encodeKey)
	if err != nil {
		return "", err
	}
	return v, nil
}

//
// put key, value data to db engine
//
func (bu *Bucket) Put(key, value string) error {
	encodeKey := strconv.Itoa(bu.ID) + SPLIT + key
	return bu.KvDB.Put(encodeKey, value)
}

//
// appned data  to engine
//
func (bu *Bucket) Append(key, value string) error {
	oldV, err := bu.Get(key)
	if err != nil {
		return err
	}
	return bu.Put(key, oldV+value)
}

//
// copy all of the data in a bucket
//
func (bu *Bucket) deepCopy() (map[string]string, error) {
	encodeKeyPrefix := strconv.Itoa(bu.ID) + SPLIT
	kvs, err := bu.KvDB.DumpPrefixKey(encodeKeyPrefix)
	if err != nil {
		return nil, err
	}
	return kvs, nil
}

//
// delete all the data in a bucket
//
func (bu *Bucket) deleteBucketData() error {
	encodeKeyPrefix := strconv.Itoa(bu.ID) + SPLIT
	return bu.KvDB.DelPrefixKeys(encodeKeyPrefix)
}
