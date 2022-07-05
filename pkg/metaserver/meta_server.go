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
	"context"

	pb "github.com/eraft-io/eraft/pkg/protocol"
)

type MetaServer struct {
	pb.UnimplementedRaftServiceServer
	pb.UnimplementedMetaServiceServer
}

func (s *MetaServer) RequestVote(ctx context.Context, req *pb.RequestVoteRequest) (*pb.RequestVoteResponse, error) {
	return nil, nil
}

func (s *MetaServer) AppendEntries(ctx context.Context, req *pb.AppendEntriesRequest) (*pb.AppendEntriesResponse, error) {
	return nil, nil
}

func (s *MetaServer) Snapshot(ctx context.Context, req *pb.InstallSnapshotRequest) (*pb.InstallSnapshotResponse, error) {
	return nil, nil
}

func (s *MetaServer) ServerGroupMeta(ctx context.Context, req *pb.ServerGroupMetaConfigRequest) (*pb.ServerGroupMetaConfigResponse, error) {
	return nil, nil
}

func (s *MetaServer) FileBlockMeta(ctx context.Context, req *pb.FileBlockMetaConfigRequest) (*pb.FileBlockMetaConfigResponse, error) {
	return nil, nil
}
