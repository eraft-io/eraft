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

	"github.com/eraft-io/eraft/pkg/consts"
	pb "github.com/eraft-io/eraft/pkg/protocol"
)

const (
	SERVER_GROUP_CONFIG_REQ = iota
	ADD_BUCKET_REQ
	DEL_BUCKET_REQ
	LIST_BUCKETS_REQ
)

func EncodeServerGroupMetaRequest(in *pb.ServerGroupMetaConfigRequest) []byte {
	var encodeBuf bytes.Buffer
	enc := gob.NewEncoder(&encodeBuf)
	enc.Encode(in)
	return encodeBuf.Bytes()
}

func DecodeServerGroupMetaRequest(in []byte) *pb.ServerGroupMetaConfigRequest {
	dec := gob.NewDecoder(bytes.NewBuffer(in))
	req := pb.ServerGroupMetaConfigRequest{}
	dec.Decode(&req)
	return &req
}

//
// EncodeBucketKey
// BUCKET_META_PREFIX + bucketId
func EncodeBucketKey(bucketId string) []byte {
	var encodedBuf bytes.Buffer
	encodedBuf.Write(consts.BUCKET_META_PREFIX)
	encodedBuf.Write([]byte(bucketId))
	return encodedBuf.Bytes()
}

// EncodeBucketKey decode bucket id and return
func DecodeBucketKey(bkey []byte) string {
	return string(bkey[len(consts.BUCKET_META_PREFIX):])
}

//
// EncodeBucket: encode bucket to bytes sequence
func EncodeBucket(bucket *pb.Bucket) []byte {
	var bucketByteSeq bytes.Buffer
	enc := gob.NewEncoder(&bucketByteSeq)
	enc.Encode(bucket)
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

// EncodeObjectKey
// OBJECT_META_PREFIX + objId
func EncodeObjectKey(objId string) []byte {
	var encodedBuf bytes.Buffer
	encodedBuf.Write(consts.OBJECT_META_PREFIX)
	encodedBuf.Write([]byte(objId))
	return encodedBuf.Bytes()
}

// DecodeObjectKey deocde object id and return
func DecodeObjectKey(okey []byte) string {
	return string(okey[len(consts.OBJECT_META_PREFIX):])
}

// EncodeObject encode object to bytes sequence
func EncodeObject(obj *pb.Object) []byte {
	var objectByteSeq bytes.Buffer
	enc := gob.NewEncoder(&objectByteSeq)
	enc.Encode(obj)
	return objectByteSeq.Bytes()
}

// DecodeObject decode object byte seq to object
func DecodeObject(seqIn []byte) *pb.Object {
	dec := gob.NewDecoder(bytes.NewBuffer(seqIn))
	obj := pb.Object{}
	dec.Decode(&obj)
	return &obj
}
