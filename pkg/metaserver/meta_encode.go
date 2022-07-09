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

package metaserver

import (
	"bytes"
	"encoding/gob"

	pb "github.com/eraft-io/eraft/pkg/protocol"
)

var BUCKET_META_PREFIX = []byte{0x01, 0x09, 0x09, 0x08}

//
// EncodeBucketKey
// encode bucket meta, store it to leveldb
// BUCKET_META_PREFIX + bucketId
func EncodeBucketKey(bucketId string) []byte {
	var encodedBuf bytes.Buffer
	encodedBuf.Write(BUCKET_META_PREFIX)
	encodedBuf.Write([]byte(bucketId))
	return encodedBuf.Bytes()
}

// EncodeBucketKey decode bucket id and return
func DecodeBucketKey(bkey []byte) string {
	return string(bkey[:len(BUCKET_META_PREFIX)])
}

//
// EncodeBucket: encode bucket to bytes sequence
func EncodeBucket(bucket *pb.Bucket) []byte {
	var bucketByteSeq bytes.Buffer
	enc := gob.NewEncoder(&bucketByteSeq)
	enc.Encode(bucketByteSeq)
	return bucketByteSeq.Bytes()
}

//
// DecodeBucket: decode byte seq to bucket
func DecodeBucket(seqIn []byte) *pb.Bucket {
	dec := gob.NewDecoder(bytes.NewBuffer(seqIn))
	bucket := pb.Bucket{}
	dec.Decode(&bucket)
	return &bucket
}
