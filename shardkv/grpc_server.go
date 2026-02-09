package shardkv

import (
	"context"

	"github.com/eraft-io/eraft/shardkvpb"
)

type ShardKVgRPCServer struct {
	shardkvpb.UnimplementedShardKVServiceServer
	kv *ShardKV
}

func NewShardKVgRPCServer(kv *ShardKV) *ShardKVgRPCServer {
	return &ShardKVgRPCServer{kv: kv}
}

func (s *ShardKVgRPCServer) Command(ctx context.Context, req *shardkvpb.CommandRequest) (*shardkvpb.CommandResponse, error) {
	args := &CommandRequest{
		Key:       req.Key,
		Value:     req.Value,
		Op:        OperationOp(req.Op),
		ClientId:  req.ClientId,
		CommandId: req.CommandId,
	}
	reply := &CommandResponse{}
	s.kv.Command(args, reply)
	return &shardkvpb.CommandResponse{
		Err:   reply.Err.String(),
		Value: reply.Value,
	}, nil
}

func (s *ShardKVgRPCServer) GetShardsData(ctx context.Context, req *shardkvpb.ShardOperationRequest) (*shardkvpb.ShardOperationResponse, error) {
	shardIDs := make([]int, len(req.ShardIds))
	for i, id := range req.ShardIds {
		shardIDs[i] = int(id)
	}
	args := &ShardOperationRequest{
		ConfigNum: int(req.ConfigNum),
		ShardIDs:  shardIDs,
	}
	reply := &ShardOperationResponse{}
	s.kv.GetShardsData(args, reply)

	respShards := make(map[int32]*shardkvpb.ShardData)
	for sid, kvMap := range reply.Shards {
		respShards[int32(sid)] = &shardkvpb.ShardData{Kv: kvMap}
	}

	respLastOps := make(map[int64]*shardkvpb.OperationContext)
	for cid, opCtx := range reply.LastOperations {
		respLastOps[cid] = &shardkvpb.OperationContext{
			MaxAppliedCommandId: opCtx.MaxAppliedCommandId,
			LastResponse: &shardkvpb.CommandResponse{
				Err:   opCtx.LastResponse.Err.String(),
				Value: opCtx.LastResponse.Value,
			},
		}
	}

	return &shardkvpb.ShardOperationResponse{
		Err:            reply.Err.String(),
		ConfigNum:      int32(reply.ConfigNum),
		Shards:         respShards,
		LastOperations: respLastOps,
	}, nil
}

func (s *ShardKVgRPCServer) DeleteShardsData(ctx context.Context, req *shardkvpb.ShardOperationRequest) (*shardkvpb.ShardOperationResponse, error) {
	shardIDs := make([]int, len(req.ShardIds))
	for i, id := range req.ShardIds {
		shardIDs[i] = int(id)
	}
	args := &ShardOperationRequest{
		ConfigNum: int(req.ConfigNum),
		ShardIDs:  shardIDs,
	}
	reply := &ShardOperationResponse{}
	s.kv.DeleteShardsData(args, reply)
	return &shardkvpb.ShardOperationResponse{
		Err: reply.Err.String(),
	}, nil
}

func (s *ShardKVgRPCServer) GetStatus(ctx context.Context, req *shardkvpb.GetStatusRequest) (*shardkvpb.GetStatusResponse, error) {
	gid, id, state, term, lastApplied, commitIndex, storageSize := s.kv.GetStatus()
	return &shardkvpb.GetStatusResponse{
		Id:          int64(id),
		State:       state,
		Term:        int64(term),
		LastApplied: int64(lastApplied),
		CommitIndex: int64(commitIndex),
		StorageSize: storageSize,
		Gid:         int64(gid),
	}, nil
}
