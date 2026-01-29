package shardkv

//
// client code to talk to a sharded key/value service.
//
// the client first talks to the shardctrler to find out
// the assignment of shards (keys) to groups, and then
// talks to the group that holds the key's shard.
//

import (
	"context"
	"crypto/rand"
	"fmt"
	"math/big"
	"strings"
	"time"

	"github.com/eraft-io/eraft/labrpc"
	"github.com/eraft-io/eraft/shardctrler"
	"github.com/eraft-io/eraft/shardkvpb"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

// which shard is a key in?
// please use this function,
// and please do not change it.
func key2shard(key string) int {
	shard := 0
	if len(key) > 0 {
		shard = int(key[0])
	}
	shard %= shardctrler.NShards
	return shard
}

func nrand() int64 {
	max := big.NewInt(int64(1) << 62)
	bigx, _ := rand.Int(rand.Reader, max)
	x := bigx.Int64()
	return x
}

type ShardKVClient interface {
	Command(ctx context.Context, in *shardkvpb.CommandRequest, opts ...grpc.CallOption) (*shardkvpb.CommandResponse, error)
	GetShardsData(ctx context.Context, in *shardkvpb.ShardOperationRequest, opts ...grpc.CallOption) (*shardkvpb.ShardOperationResponse, error)
	DeleteShardsData(ctx context.Context, in *shardkvpb.ShardOperationRequest, opts ...grpc.CallOption) (*shardkvpb.ShardOperationResponse, error)
	GetStatus(ctx context.Context, in *shardkvpb.GetStatusRequest, opts ...grpc.CallOption) (*shardkvpb.GetStatusResponse, error)
}

type gRPCShardKVClient struct {
	client shardkvpb.ShardKVServiceClient
}

func (c *gRPCShardKVClient) Command(ctx context.Context, in *shardkvpb.CommandRequest, opts ...grpc.CallOption) (*shardkvpb.CommandResponse, error) {
	return c.client.Command(ctx, in, opts...)
}

func (c *gRPCShardKVClient) GetShardsData(ctx context.Context, in *shardkvpb.ShardOperationRequest, opts ...grpc.CallOption) (*shardkvpb.ShardOperationResponse, error) {
	return c.client.GetShardsData(ctx, in, opts...)
}

func (c *gRPCShardKVClient) DeleteShardsData(ctx context.Context, in *shardkvpb.ShardOperationRequest, opts ...grpc.CallOption) (*shardkvpb.ShardOperationResponse, error) {
	return c.client.DeleteShardsData(ctx, in, opts...)
}

func (c *gRPCShardKVClient) GetStatus(ctx context.Context, in *shardkvpb.GetStatusRequest, opts ...grpc.CallOption) (*shardkvpb.GetStatusResponse, error) {
	return c.client.GetStatus(ctx, in, opts...)
}

type LabrpcShardKVClient struct {
	end *labrpc.ClientEnd
}

func (c *LabrpcShardKVClient) Command(ctx context.Context, in *shardkvpb.CommandRequest, opts ...grpc.CallOption) (*shardkvpb.CommandResponse, error) {
	args := &CommandRequest{
		Key:       in.Key,
		Value:     in.Value,
		Op:        OperationOp(in.Op),
		ClientId:  in.ClientId,
		CommandId: in.CommandId,
	}
	reply := &CommandResponse{}
	if ok := c.end.Call("ShardKV.Command", args, reply); ok {
		return &shardkvpb.CommandResponse{
			Err:   reply.Err.String(),
			Value: reply.Value,
		}, nil
	}
	return nil, fmt.Errorf("rpc failed")
}

func (c *LabrpcShardKVClient) GetShardsData(ctx context.Context, in *shardkvpb.ShardOperationRequest, opts ...grpc.CallOption) (*shardkvpb.ShardOperationResponse, error) {
	shardIDs := make([]int, len(in.ShardIds))
	for i, id := range in.ShardIds {
		shardIDs[i] = int(id)
	}
	args := &ShardOperationRequest{
		ConfigNum: int(in.ConfigNum),
		ShardIDs:  shardIDs,
	}
	reply := &ShardOperationResponse{}
	if ok := c.end.Call("ShardKV.GetShardsData", args, reply); ok {
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
	return nil, fmt.Errorf("rpc failed")
}

func (c *LabrpcShardKVClient) DeleteShardsData(ctx context.Context, in *shardkvpb.ShardOperationRequest, opts ...grpc.CallOption) (*shardkvpb.ShardOperationResponse, error) {
	shardIDs := make([]int, len(in.ShardIds))
	for i, id := range in.ShardIds {
		shardIDs[i] = int(id)
	}
	args := &ShardOperationRequest{
		ConfigNum: int(in.ConfigNum),
		ShardIDs:  shardIDs,
	}
	reply := &ShardOperationResponse{}
	if ok := c.end.Call("ShardKV.DeleteShardsData", args, reply); ok {
		return &shardkvpb.ShardOperationResponse{
			Err: reply.Err.String(),
		}, nil
	}
	return nil, fmt.Errorf("rpc failed")
}

func (c *LabrpcShardKVClient) GetStatus(ctx context.Context, in *shardkvpb.GetStatusRequest, opts ...grpc.CallOption) (*shardkvpb.GetStatusResponse, error) {
	return nil, fmt.Errorf("GetStatus not supported in labrpc mode")
}

type Clerk struct {
	sm        *shardctrler.Clerk
	config    shardctrler.Config
	clients   map[int][]ShardKVClient
	leaderIds map[int]int
	clientId  int64
	commandId int64
	makeEnd   func(string) *labrpc.ClientEnd
}

// the tester calls MakeClerk.
//
// ctrlers[] is needed to call shardctrler.MakeClerk().

func MakeClerk(ctrlers []string) *Clerk {
	ck := &Clerk{
		sm:        shardctrler.MakeClerk(ctrlers),
		clients:   make(map[int][]ShardKVClient),
		leaderIds: make(map[int]int),
		clientId:  nrand(),
		commandId: 0,
	}
	ck.config = ck.sm.Query(-1)
	return ck
}

func MakeLabrpcClerk(ctrlers []*labrpc.ClientEnd, makeEnd func(string) *labrpc.ClientEnd) *Clerk {
	ck := &Clerk{
		sm:        shardctrler.MakeLabrpcClerk(ctrlers),
		clients:   make(map[int][]ShardKVClient),
		leaderIds: make(map[int]int),
		clientId:  nrand(),
		commandId: 0,
		makeEnd:   makeEnd,
	}
	ck.config = ck.sm.Query(-1)
	return ck
}

// fetch the current value for a key.
// returns "" if the key does not exist.
// keeps trying forever in the face of all other errors.
// You will have to modify this function.
func (ck *Clerk) Get(key string) string {
	return ck.Command(&CommandRequest{Key: key, Op: OpGet})
}

func (ck *Clerk) Put(key string, value string) {
	ck.Command(&CommandRequest{Key: key, Value: value, Op: OpPut})
}

func (ck *Clerk) Append(key string, value string) {
	ck.Command(&CommandRequest{Key: key, Value: value, Op: OpAppend})
}

func (ck *Clerk) Command(request *CommandRequest) string {
	request.ClientId, request.CommandId = ck.clientId, ck.commandId
	for {
		shard := key2shard(request.Key)
		gid := ck.config.Shards[shard]
		if servers, ok := ck.config.Groups[gid]; ok {
			if _, ok = ck.leaderIds[gid]; !ok {
				ck.leaderIds[gid] = 0
			}
			if _, ok = ck.clients[gid]; !ok {
				ck.clients[gid] = make([]ShardKVClient, len(servers))
				for i, srv := range servers {
					// Check if it's a gRPC address or labrpc
					if strings.Contains(srv, ":") || strings.HasPrefix(srv, "localhost") {
						conn, err := grpc.Dial(srv, grpc.WithTransportCredentials(insecure.NewCredentials()))
						if err == nil {
							ck.clients[gid][i] = &gRPCShardKVClient{client: shardkvpb.NewShardKVServiceClient(conn)}
						}
					} else {
						if ck.makeEnd != nil {
							ck.clients[gid][i] = &LabrpcShardKVClient{end: ck.makeEnd(srv)}
						}
					}
				}
			}

			oldLeaderId := ck.leaderIds[gid]
			newLeaderId := oldLeaderId
			for {
				req := &shardkvpb.CommandRequest{
					Key:       request.Key,
					Value:     request.Value,
					Op:        shardkvpb.Op(request.Op),
					ClientId:  request.ClientId,
					CommandId: request.CommandId,
				}
				ctx, cancel := context.WithTimeout(context.Background(), ExecuteTimeout)
				resp, err := ck.clients[gid][newLeaderId].Command(ctx, req)
				cancel()

				if err == nil && (resp.Err == OK.String() || resp.Err == ErrNoKey.String()) {
					ck.commandId++
					return resp.Value
				} else if err == nil && resp.Err == ErrWrongGroup.String() {
					break
				} else {
					newLeaderId = (newLeaderId + 1) % len(servers)
					if newLeaderId == oldLeaderId {
						break
					}
					continue
				}
			}
		}
		time.Sleep(100 * time.Millisecond)
		ck.config = ck.sm.Query(-1)
		// Reset clients for this gid as config might have changed
		delete(ck.clients, gid)
	}
}

func (ck *Clerk) GetStatus() []*shardkvpb.GetStatusResponse {
	results := make([]*shardkvpb.GetStatusResponse, 0)
	for gid, groupClients := range ck.clients {
		for i, client := range groupClients {
			ctx, cancel := context.WithTimeout(context.Background(), 100*time.Millisecond)
			resp, err := client.GetStatus(ctx, &shardkvpb.GetStatusRequest{})
			cancel()
			if err == nil {
				results = append(results, resp)
			} else {
				results = append(results, &shardkvpb.GetStatusResponse{Id: int64(gid*100 + i), State: "Offline"})
			}
		}
	}
	return results
}
