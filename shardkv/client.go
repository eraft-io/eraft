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
	"math/big"
	"time"

	"github.com/eraft-io/eraft/labrpc"
	"github.com/eraft-io/eraft/raftpb"
	"github.com/eraft-io/eraft/shardctrler"
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

type Clerk struct {
	sm        *shardctrler.Clerk
	config    shardctrler.Config
	makeEnd   func(string) *labrpc.ClientEnd
	leaderIds map[int]int
	cliendId  int64
	commandId int64
}

// the tester calls MakeClerk.
//
// ctrlers[] is needed to call shardctrler.MakeClerk().

func MakeClerk(ctrlers []*labrpc.ClientEnd, makeEnd func(string) *labrpc.ClientEnd) *Clerk {
	ck := &Clerk{
		sm:        shardctrler.MakeClerk(ctrlers),
		makeEnd:   makeEnd,
		leaderIds: make(map[int]int),
		cliendId:  nrand(),
		commandId: 0,
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
	request.ClientId, request.CommandId = ck.cliendId, ck.commandId
	for {
		shard := key2shard(request.Key)
		gid := ck.config.Shards[shard]
		if servers, ok := ck.config.Groups[gid]; ok {
			if _, ok = ck.leaderIds[gid]; !ok {
				ck.leaderIds[gid] = 0
			}
			oldLeaderId := ck.leaderIds[gid]
			newLeaderId := oldLeaderId
			for {
				var response CommandResponse
				if ck.makeEnd(servers[newLeaderId]).GrpcClient == nil {
					req := &raftpb.CommandRequest{
						OpType:    raftpb.OpType(request.Op),
						Key:       request.Key,
						Value:     request.Value,
						ClientId:  request.ClientId,
						CommandId: request.CommandId,
					}
					resp, err := (*ck.makeEnd(servers[newLeaderId]).GrpcClient.SvrCli).CommandRPC(context.Background(), req)
					if err != nil && (resp.ErrCode == int64(ErrNoKey) || resp.ErrCode == int64(OK)) {
						ck.commandId++
						return resp.Value
					} else if err != nil || resp.ErrCode == int64(ErrWrongGroup) {
						break
					} else {
						newLeaderId = (newLeaderId + 1) % len(servers)
						if newLeaderId == oldLeaderId {
							break
						}
						continue
					}
				} else {
					ok := ck.makeEnd(servers[newLeaderId]).Call("ShardKV.Command", request, &response)
					if ok && (response.Err == OK || response.Err == ErrNoKey) {
						ck.commandId++
						return response.Value
					} else if ok && response.Err == ErrWrongGroup {
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
		}
		time.Sleep(100 * time.Millisecond)
		ck.config = ck.sm.Query(-1)
	}
}
