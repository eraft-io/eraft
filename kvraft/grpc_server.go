package kvraft

import (
	"context"

	"github.com/eraft-io/eraft/kvraftpb"
)

type KVgRPCServer struct {
	kvraftpb.UnimplementedKVServiceServer
	kv *KVServer
}

func NewKVgRPCServer(kv *KVServer) *KVgRPCServer {
	return &KVgRPCServer{kv: kv}
}

func (s *KVgRPCServer) Command(ctx context.Context, req *kvraftpb.CommandRequest) (*kvraftpb.CommandResponse, error) {
	args := &CommandRequest{
		Key:       req.Key,
		Value:     req.Value,
		Op:        OperationOp(req.Op),
		ClientId:  req.ClientId,
		CommandId: req.CommandId,
	}
	reply := &CommandResponse{}
	s.kv.Command(args, reply)
	return &kvraftpb.CommandResponse{
		Err:   reply.Err.String(),
		Value: reply.Value,
	}, nil
}

func (s *KVgRPCServer) GetStatus(ctx context.Context, req *kvraftpb.GetStatusRequest) (*kvraftpb.GetStatusResponse, error) {
	id, state, term, lastApplied, commitIndex, storageSize := s.kv.GetStatus()
	return &kvraftpb.GetStatusResponse{
		Id:          int64(id),
		State:       state,
		Term:        int64(term),
		LastApplied: int64(lastApplied),
		CommitIndex: int64(commitIndex),
		StorageSize: storageSize,
	}, nil
}
