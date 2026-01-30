package shardctrler

import (
	"context"

	"github.com/eraft-io/eraft/shardctrlerpb"
)

type ShardCtrlergRPCServer struct {
	shardctrlerpb.UnimplementedShardCtrlerServiceServer
	sc *ShardCtrler
}

func NewShardCtrlergRPCServer(sc *ShardCtrler) *ShardCtrlergRPCServer {
	return &ShardCtrlergRPCServer{sc: sc}
}

func (s *ShardCtrlergRPCServer) Command(ctx context.Context, req *shardctrlerpb.CommandRequest) (*shardctrlerpb.CommandResponse, error) {
	servers := make(map[int][]string)
	for gid, srvs := range req.Servers {
		servers[int(gid)] = srvs.Servers
	}

	gids := make([]int, len(req.Gids))
	for i, gid := range req.Gids {
		gids[i] = int(gid)
	}

	args := &CommandRequest{
		Servers:   servers,
		GIDs:      gids,
		Shard:     int(req.Shard),
		GID:       int(req.Gid),
		Num:       int(req.Num),
		Op:        OperationOp(req.Op),
		ClientId:  req.ClientId,
		CommandId: req.CommandId,
	}
	reply := &CommandResponse{}
	s.sc.Command(args, reply)

	respConfig := &shardctrlerpb.Config{
		Num:    int64(reply.Config.Num),
		Shards: make([]int32, NShards),
		Groups: make(map[int32]*shardctrlerpb.Servers),
	}
	for i, gid := range reply.Config.Shards {
		respConfig.Shards[i] = int32(gid)
	}
	for gid, srvs := range reply.Config.Groups {
		respConfig.Groups[int32(gid)] = &shardctrlerpb.Servers{Servers: srvs}
	}

	return &shardctrlerpb.CommandResponse{
		Err:    reply.Err.String(),
		Config: respConfig,
	}, nil
}

func (s *ShardCtrlergRPCServer) GetStatus(ctx context.Context, req *shardctrlerpb.GetStatusRequest) (*shardctrlerpb.GetStatusResponse, error) {
	id, state, term, lastApplied, commitIndex, storageSize := s.sc.GetStatus()
	return &shardctrlerpb.GetStatusResponse{
		Id:          int64(id),
		State:       state,
		Term:        int64(term),
		LastApplied: int64(lastApplied),
		CommitIndex: int64(commitIndex),
		StorageSize: storageSize,
	}, nil
}
