package blockserver

import (
	"context"

	pb "github.com/eraft-io/eraft/pkg/protocol"
)

type BlockServer struct {
	pb.UnimplementedRaftServiceServer
	pb.UnimplementedFileBlockServiceServer
}

func (s *BlockServer) RequestVote(ctx context.Context, req *pb.RequestVoteRequest) (*pb.RequestVoteResponse, error) {
	return nil, nil
}

func (s *BlockServer) AppendEntries(ctx context.Context, req *pb.AppendEntriesRequest) (*pb.AppendEntriesResponse, error) {
	return nil, nil
}

func (s *BlockServer) Snapshot(ctx context.Context, req *pb.InstallSnapshotRequest) (*pb.InstallSnapshotResponse, error) {
	return nil, nil
}

func (s *BlockServer) WriteFileBlock(ctx context.Context, req *pb.WriteFileBlockRequest) (*pb.WriteFileBlockResponse, error) {
	return nil, nil
}

func (s *BlockServer) ReadFileBlock(ctx context.Context, req *pb.ReadFileBlockRequest) (*pb.ReadFileBlockResponse, error) {
	return nil, nil
}
