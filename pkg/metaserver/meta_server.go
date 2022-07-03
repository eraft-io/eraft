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
