# Copyright [2022] [WellWood] [wellwood-x@googlegroups.com]

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

# 	http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#!/bin/sh
export PATH="$PATH:$(go env GOPATH)/bin"

go install google.golang.org/protobuf/cmd/protoc-gen-go@v1.28
go install google.golang.org/grpc/cmd/protoc-gen-go-grpc@v1.2

protoc -I ../pkg/protocol ../pkg/protocol/raftbasic.proto --go_out=../pkg/protocol --go-grpc_out=../pkg/protocol
protoc -I ../pkg/protocol ../pkg/protocol/wellwood.proto --go_out=../pkg/protocol --go-grpc_out=../pkg/protocol
