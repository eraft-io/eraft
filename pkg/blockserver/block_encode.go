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

package blockserver

import (
	"bytes"
	"encoding/gob"

	pb "github.com/eraft-io/eraft/pkg/protocol"
)

func EncodeBlockServerRequest(in *pb.FileBlockOpRequest) []byte {
	var encodeBuf bytes.Buffer
	enc := gob.NewEncoder(&encodeBuf)
	enc.Encode(in)
	return encodeBuf.Bytes()
}

func DecodeBlockServerRequest(in []byte) *pb.FileBlockOpRequest {
	dec := gob.NewDecoder(bytes.NewBuffer(in))
	req := pb.FileBlockOpRequest{}
	dec.Decode(&req)
	return &req
}
