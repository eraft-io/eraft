package shardkv

import (
	"bytes"
	"context"
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"github.com/eraft-io/eraft/labgob"
	"github.com/eraft-io/eraft/labrpc"
	"github.com/eraft-io/eraft/raft"
	"github.com/eraft-io/eraft/shardctrler"
	"github.com/eraft-io/eraft/shardkvpb"
	"github.com/syndtr/goleveldb/leveldb"
	"github.com/syndtr/goleveldb/leveldb/util"
	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"
)

type LevelDBShardStore struct {
	db   *leveldb.DB
	path string
}

func NewLevelDBShardStore(path string) *LevelDBShardStore {
	db, err := leveldb.OpenFile(path, nil)
	if err != nil {
		panic(err)
	}
	return &LevelDBShardStore{db: db, path: path}
}

func (ls *LevelDBShardStore) Get(shardID int, key string) (string, Err) {
	val, err := ls.db.Get([]byte(fmt.Sprintf("s_%d_%s", shardID, key)), nil)
	if err == leveldb.ErrNotFound {
		return "", ErrNoKey
	}
	return string(val), OK
}

func (ls *LevelDBShardStore) Put(shardID int, key, value string) Err {
	ls.db.Put([]byte(fmt.Sprintf("s_%d_%s", shardID, key)), []byte(value), nil)
	return OK
}

func (ls *LevelDBShardStore) Append(shardID int, key, value string) Err {
	oldVal, _ := ls.db.Get([]byte(fmt.Sprintf("s_%d_%s", shardID, key)), nil)
	newVal := append(oldVal, []byte(value)...)
	ls.db.Put([]byte(fmt.Sprintf("s_%d_%s", shardID, key)), newVal, nil)
	return OK
}

func (ls *LevelDBShardStore) Close() {
	ls.db.Close()
}

func (ls *LevelDBShardStore) Size() int64 {
	var size int64
	filepath.Walk(ls.path, func(_ string, info os.FileInfo, err error) error {
		if err != nil {
			return err
		}
		if !info.IsDir() {
			size += info.Size()
		}
		return nil
	})
	return size
}

type ShardKV struct {
	mu      sync.RWMutex
	dead    int32
	rf      *raft.Raft
	applyCh chan raft.ApplyMsg

	gid int // The group ID of the current server
	sc  *shardctrler.Clerk

	maxRaftState int // snapshot if log grows this big
	lastApplied  int // record the lastApplied to prevent stateMachine from rollback

	lastConfig    shardctrler.Config
	currentConfig shardctrler.Config

	// Data storage
	store *LevelDBShardStore

	// Memory state for non-persistent or easily restorable data
	shardStatus    [shardctrler.NShards]ShardStatus
	lastOperations map[int64]OperationContext    // determine whether log is duplicated by recording the last commandId and response corresponding to the clientId
	notifyChans    map[int]chan *CommandResponse // notify client goroutine by applier goroutine to response
	makeEnd        func(string) *labrpc.ClientEnd
}

func (kv *ShardKV) Raft() *raft.Raft {
	return kv.rf
}

func (kv *ShardKV) GetStatus() (int, int, string, int, int, int, int64) {
	id, state, term, lastApplied, commitIndex := kv.rf.GetStatus()
	return kv.gid, id, state, term, lastApplied, commitIndex, kv.store.Size() + int64(kv.rf.GetRaftStateSize())
}

func (kv *ShardKV) Command(request *CommandRequest, response *CommandResponse) {
	kv.mu.RLock()
	// return result directly without raft layer's participation if request is duplicated
	if request.Op != OpGet && kv.isDuplicateRequest(request.ClientId, request.CommandId) {
		lastResponse := kv.lastOperations[request.ClientId].LastResponse
		response.Value, response.Err = lastResponse.Value, lastResponse.Err
		kv.mu.RUnlock()
		return
	}
	// return ErrWrongGroup directly to let client fetch latest configuration and perform a retry if this key can't be served by this shard at present
	if !kv.canServe(key2shard(request.Key)) {
		response.Err = ErrWrongGroup
		kv.mu.RUnlock()
		return
	}
	kv.mu.RUnlock()
	kv.Execute(NewOperationCommand(request), response)
}

func (kv *ShardKV) Execute(command Command, response *CommandResponse) {
	// do not hold lock to improve throughput
	// when KVServer holds the lock to take snapshot, underlying raft can still commit raft logs
	w := new(bytes.Buffer)
	e := labgob.NewEncoder(w)
	e.Encode(command)
	index, _, isLeader := kv.rf.Start(w.Bytes())
	if !isLeader {
		response.Err = ErrWrongLeader
		return
	}
	defer DPrintf("{Node %v}{Group %v} processes Command %v with CommandResponse %v", kv.rf.Me(), kv.gid, command, response)
	kv.mu.Lock()
	ch := kv.getNotifyChan(index)
	kv.mu.Unlock()
	select {
	case result := <-ch:
		response.Value, response.Err = result.Value, result.Err
	case <-time.After(ExecuteTimeout):
		response.Err = ErrTimeout
	}
	// release notifyChan to reduce memory footprint
	// why asynchronously? to improve throughput, here is no need to block client request
	go func() {
		kv.mu.Lock()
		kv.removeOutdatedNotifyChan(index)
		kv.mu.Unlock()
	}()
}

func (kv *ShardKV) GetShardsData(request *ShardOperationRequest, response *ShardOperationResponse) {
	// only pull shards from leader
	if _, isLeader := kv.rf.GetState(); !isLeader {
		response.Err = ErrWrongLeader
		return
	}
	kv.mu.RLock()
	defer kv.mu.RUnlock()
	defer DPrintf("{Node %v}{Group %v} processes PullTaskRequest %v with response %v", kv.rf.Me(), kv.gid, request, response)

	if kv.currentConfig.Num < request.ConfigNum {
		response.Err = ErrNotReady
		return
	}

	response.Shards = make(map[int]map[string]string)
	for _, shardID := range request.ShardIDs {
		shardData := make(map[string]string)
		prefix := []byte(fmt.Sprintf("s_%d_", shardID))
		iter := kv.store.db.NewIterator(util.BytesPrefix(prefix), nil)
		for iter.Next() {
			key := string(iter.Key())[len(prefix):]
			shardData[key] = string(iter.Value())
		}
		iter.Release()
		response.Shards[shardID] = shardData
	}

	response.LastOperations = make(map[int64]OperationContext)
	for clientID, operation := range kv.lastOperations {
		response.LastOperations[clientID] = operation.deepCopy()
	}

	response.ConfigNum, response.Err = request.ConfigNum, OK
}

func (kv *ShardKV) DeleteShardsData(request *ShardOperationRequest, response *ShardOperationResponse) {
	// only delete shards when role is leader
	if _, isLeader := kv.rf.GetState(); !isLeader {
		response.Err = ErrWrongLeader
		return
	}

	defer DPrintf("{Node %v}{Group %v} processes GCTaskRequest %v with response %v", kv.rf.Me(), kv.gid, request, response)

	kv.mu.RLock()
	if kv.currentConfig.Num > request.ConfigNum {
		DPrintf("{Node %v}{Group %v}'s encounters duplicated shards deletion %v when currentConfig is %v", kv.rf.Me(), kv.gid, request, kv.currentConfig)
		response.Err = OK
		kv.mu.RUnlock()
		return
	}
	kv.mu.RUnlock()

	var commandResponse CommandResponse
	kv.Execute(NewDeleteShardsCommand(request), &commandResponse)

	response.Err = commandResponse.Err
}

// each RPC imply that the client has seen the reply for its previous RPC
// therefore, we only need to determine whether the latest commandId of a clientId meets the criteria
func (kv *ShardKV) isDuplicateRequest(clientId int64, requestId int64) bool {
	operationContext, ok := kv.lastOperations[clientId]
	return ok && requestId <= operationContext.MaxAppliedCommandId
}

// check whether this raft group can serve this shard at present
func (kv *ShardKV) canServe(shardID int) bool {
	// Only the current server responsible for this shard's data will serve it
	return kv.currentConfig.Shards[shardID] == kv.gid && (kv.shardStatus[shardID] == Serving || kv.shardStatus[shardID] == GCing)
}

// the tester calls Kill() when a ShardKV instance won't
// be needed again. you are not required to do anything
// in Kill(), but it might be convenient to (for example)
// turn off debug output from this instance.
func (kv *ShardKV) Kill() {
	DPrintf("{Node %v}{Group %v} has been killed", kv.rf.Me(), kv.gid)
	atomic.StoreInt32(&kv.dead, 1)
	kv.rf.Kill()
	kv.store.Close()
}

func (kv *ShardKV) killed() bool {
	return atomic.LoadInt32(&kv.dead) == 1
}

// a dedicated applier goroutine to apply committed entries to stateMachine, take snapshot and apply snapshot from raft
func (kv *ShardKV) applier() {
	for kv.killed() == false {
		select {
		case message := <-kv.applyCh:
			DPrintf("{Node %v}{Group %v} tries to apply message %v", kv.rf.Me(), kv.gid, message)
			if message.CommandValid {
				kv.mu.Lock()
				if message.CommandIndex <= kv.lastApplied {
					DPrintf("{Node %v}{Group %v} discards outdated message %v because a newer snapshot which lastApplied is %v has been restored", kv.rf.Me(), kv.gid, message, kv.lastApplied)
					kv.mu.Unlock()
					continue
				}
				kv.lastApplied = message.CommandIndex

				var response *CommandResponse
				var command Command
				commandBytes, ok := message.Command.([]byte)
				if !ok {
					// Fallback: if it's not []byte, it might be the raw Command struct (from old code)
					command = message.Command.(Command)
				} else {
					r := bytes.NewBuffer(commandBytes)
					d := labgob.NewDecoder(r)
					d.Decode(&command)
				}

				switch command.Op {
				case Operation:
					var operation CommandRequest
					r2 := bytes.NewBuffer(command.Data)
					d2 := labgob.NewDecoder(r2)
					d2.Decode(&operation)
					response = kv.applyOperation(&message, &operation)
				case Configuration:
					var nextConfig shardctrler.Config
					r2 := bytes.NewBuffer(command.Data)
					d2 := labgob.NewDecoder(r2)
					d2.Decode(&nextConfig)
					response = kv.applyConfiguration(&nextConfig)
				case InsertShards:
					var shardsInfo ShardOperationResponse
					r2 := bytes.NewBuffer(command.Data)
					d2 := labgob.NewDecoder(r2)
					d2.Decode(&shardsInfo)
					response = kv.applyInsertShards(&shardsInfo)
				case DeleteShards:
					var shardsInfo ShardOperationRequest
					r2 := bytes.NewBuffer(command.Data)
					d2 := labgob.NewDecoder(r2)
					d2.Decode(&shardsInfo)
					response = kv.applyDeleteShards(&shardsInfo)
				case EmptyEntry:
					response = kv.applyEmptyEntry()
				}

				// only notify related channel for currentTerm's log when node is leader
				if currentTerm, isLeader := kv.rf.GetState(); isLeader && message.CommandTerm == currentTerm {
					ch := kv.getNotifyChan(message.CommandIndex)
					ch <- response
				}

				needSnapshot := kv.needSnapshot()
				if needSnapshot {
					kv.takeSnapshot(message.CommandIndex)
				}
				kv.mu.Unlock()
			} else if message.SnapshotValid {
				kv.mu.Lock()
				if kv.rf.CondInstallSnapshot(message.SnapshotTerm, message.SnapshotIndex, message.Snapshot) {
					kv.restoreSnapshot(message.Snapshot)
					kv.lastApplied = message.SnapshotIndex
				}
				kv.mu.Unlock()
			} else {
				panic(fmt.Sprintf("unexpected Message %v", message))
			}
		}
	}
}

func (kv *ShardKV) applyOperation(message *raft.ApplyMsg, operation *CommandRequest) *CommandResponse {
	var response *CommandResponse
	// Mapping from key to shardID
	shardID := key2shard(operation.Key)
	if kv.canServe(shardID) {
		if operation.Op != OpGet && kv.isDuplicateRequest(operation.ClientId, operation.CommandId) {
			DPrintf("{Node %v}{Group %v} doesn't apply duplicated message %v to stateMachine because maxAppliedCommandId is %v for client %v", kv.rf.Me(), kv.gid, message, kv.lastOperations[operation.ClientId], operation.ClientId)
			return kv.lastOperations[operation.ClientId].LastResponse
		} else {
			response = kv.applyLogToStateMachines(operation, shardID)
			if operation.Op != OpGet {
				kv.lastOperations[operation.ClientId] = OperationContext{operation.CommandId, response}
			}
			return response
		}
	}
	return &CommandResponse{ErrWrongGroup, ""}
}

// Apply configuration information to the server to ensure consistency across all service nodes in the group
func (kv *ShardKV) applyConfiguration(nextConfig *shardctrler.Config) *CommandResponse {
	if nextConfig.Num == kv.currentConfig.Num+1 {
		DPrintf("{Node %v}{Group %v} updates currentConfig from %v to %v", kv.rf.Me(), kv.gid, kv.currentConfig, nextConfig)
		kv.updateShardStatus(nextConfig)
		kv.lastConfig = kv.currentConfig
		kv.currentConfig = *nextConfig
		return &CommandResponse{OK, ""}
	}
	DPrintf("{Node %v}{Group %v} rejects outdated config %v when currentConfig is %v", kv.rf.Me(), kv.gid, nextConfig, kv.currentConfig)
	return &CommandResponse{ErrOutDated, ""}
}

func (kv *ShardKV) applyInsertShards(shardsInfo *ShardOperationResponse) *CommandResponse {
	if shardsInfo.ConfigNum == kv.currentConfig.Num {
		DPrintf("{Node %v}{Group %v} accepts shards insertion %v when currentConfig is %v", kv.rf.Me(), kv.gid, shardsInfo, kv.currentConfig)
		for shardId, shardData := range shardsInfo.Shards {
			if kv.shardStatus[shardId] == Pulling {
				for key, value := range shardData {
					kv.store.Put(shardId, key, value)
				}
				kv.shardStatus[shardId] = GCing
			} else {
				DPrintf("{Node %v}{Group %v} encounters duplicated shards insertion %v when currentConfig is %v", kv.rf.Me(), kv.gid, shardsInfo, kv.currentConfig)
				break
			}
		}
		for clientId, operationContext := range shardsInfo.LastOperations {
			if lastOperation, ok := kv.lastOperations[clientId]; !ok || lastOperation.MaxAppliedCommandId < operationContext.MaxAppliedCommandId {
				kv.lastOperations[clientId] = operationContext
			}
		}
		return &CommandResponse{OK, ""}
	}
	DPrintf("{Node %v}{Group %v} rejects outdated shards insertion %v when currentConfig is %v", kv.rf.Me(), kv.gid, shardsInfo, kv.currentConfig)
	return &CommandResponse{ErrOutDated, ""}
}

func (kv *ShardKV) applyDeleteShards(shardsInfo *ShardOperationRequest) *CommandResponse {
	if shardsInfo.ConfigNum == kv.currentConfig.Num {
		DPrintf("{Node %v}{Group %v}'s shards status are %v before accepting shards deletion %v when currentConfig is %v", kv.rf.Me(), kv.gid, kv.getShardStatus(), shardsInfo, kv.currentConfig)
		for _, shardId := range shardsInfo.ShardIDs {
			if kv.shardStatus[shardId] == GCing {
				kv.shardStatus[shardId] = Serving
			} else if kv.shardStatus[shardId] == BePulling {
				kv.shardStatus[shardId] = Serving // Reset to serving if it was BePulling and now deleted
				// Actually, if it was BePulling, we should clear the data in LevelDB for this shard
				kv.clearShardData(shardId)
			} else {
				DPrintf("{Node %v}{Group %v} encounters duplicated shards deletion %v when currentConfig is %v", kv.rf.Me(), kv.gid, shardsInfo, kv.currentConfig)
				break
			}
		}
		DPrintf("{Node %v}{Group %v}'s shards status are %v after accepting shards deletion %v when currentConfig is %v", kv.rf.Me(), kv.gid, kv.getShardStatus(), shardsInfo, kv.currentConfig)
		return &CommandResponse{OK, ""}
	}
	DPrintf("{Node %v}{Group %v}'s encounters duplicated shards deletion %v when currentConfig is %v", kv.rf.Me(), kv.gid, shardsInfo, kv.currentConfig)
	return &CommandResponse{OK, ""}
}

func (kv *ShardKV) clearShardData(shardID int) {
	prefix := []byte(fmt.Sprintf("s_%d_", shardID))
	iter := kv.store.db.NewIterator(util.BytesPrefix(prefix), nil)
	batch := new(leveldb.Batch)
	for iter.Next() {
		batch.Delete(iter.Key())
	}
	iter.Release()
	kv.store.db.Write(batch, nil)
}

func (kv *ShardKV) applyEmptyEntry() *CommandResponse {
	return &CommandResponse{OK, ""}
}

func (kv *ShardKV) getShardStatus() []ShardStatus {
	results := make([]ShardStatus, shardctrler.NShards)
	for i := 0; i < shardctrler.NShards; i++ {
		results[i] = kv.shardStatus[i]
	}
	return results
}

func (kv *ShardKV) updateShardStatus(nextConfig *shardctrler.Config) {
	for i := 0; i < shardctrler.NShards; i++ {
		if kv.currentConfig.Shards[i] != kv.gid && nextConfig.Shards[i] == kv.gid {
			gid := kv.currentConfig.Shards[i]
			if gid != 0 {
				kv.shardStatus[i] = Pulling
			}
		}
		if kv.currentConfig.Shards[i] == kv.gid && nextConfig.Shards[i] != kv.gid {
			gid := nextConfig.Shards[i]
			if gid != 0 {
				kv.shardStatus[i] = BePulling
			}
		}
	}
}

func (kv *ShardKV) getShardIDsByStatus(status ShardStatus) map[int][]int {
	gid2shardIDs := make(map[int][]int)
	for i, s := range kv.shardStatus {
		if s == status {
			gid := kv.lastConfig.Shards[i]
			if gid != 0 {
				if _, ok := gid2shardIDs[gid]; !ok {
					gid2shardIDs[gid] = make([]int, 0)
				}
				gid2shardIDs[gid] = append(gid2shardIDs[gid], i)
			}
		}
	}
	return gid2shardIDs
}

func (kv *ShardKV) needSnapshot() bool {
	return kv.maxRaftState != -1 && kv.rf.GetRaftStateSize() >= kv.maxRaftState
}

func (kv *ShardKV) takeSnapshot(index int) {
	// For ShardKV with LevelDB, we iterate over the entire DB to create a snapshot
	// This is not the most efficient way but it's consistent with how we handled kvraft
	allData := make(map[string]string)
	iter := kv.store.db.NewIterator(nil, nil)
	for iter.Next() {
		allData[string(iter.Key())] = string(iter.Value())
	}
	iter.Release()

	w := new(bytes.Buffer)
	e := labgob.NewEncoder(w)
	e.Encode(allData)
	e.Encode(kv.shardStatus)
	e.Encode(kv.lastOperations)
	e.Encode(kv.currentConfig)
	e.Encode(kv.lastConfig)
	kv.rf.Snapshot(index, w.Bytes())
}

func (kv *ShardKV) restoreSnapshot(snapshot []byte) {
	if snapshot == nil || len(snapshot) == 0 {
		kv.initStateMachines()
		return
	}
	r := bytes.NewBuffer(snapshot)
	d := labgob.NewDecoder(r)
	var allData map[string]string
	var shardStatus [shardctrler.NShards]ShardStatus
	var lastOperations map[int64]OperationContext
	var currentConfig shardctrler.Config
	var lastConfig shardctrler.Config
	if d.Decode(&allData) != nil ||
		d.Decode(&shardStatus) != nil ||
		d.Decode(&lastOperations) != nil ||
		d.Decode(&currentConfig) != nil ||
		d.Decode(&lastConfig) != nil {
		DPrintf("{Node %v}{Group %v} restores snapshot failed", kv.rf.Me(), kv.gid)
		return
	}

	// Restore LevelDB
	batch := new(leveldb.Batch)
	for k, v := range allData {
		batch.Put([]byte(k), []byte(v))
	}
	kv.store.db.Write(batch, nil)

	kv.shardStatus, kv.lastOperations, kv.currentConfig, kv.lastConfig = shardStatus, lastOperations, currentConfig, lastConfig
}

func (kv *ShardKV) initStateMachines() {
	for i := 0; i < shardctrler.NShards; i++ {
		kv.shardStatus[i] = Serving
	}
}

func (kv *ShardKV) getNotifyChan(index int) chan *CommandResponse {
	if _, ok := kv.notifyChans[index]; !ok {
		kv.notifyChans[index] = make(chan *CommandResponse, 1)
	}
	return kv.notifyChans[index]
}

func (kv *ShardKV) removeOutdatedNotifyChan(index int) {
	delete(kv.notifyChans, index)
}

func (kv *ShardKV) applyLogToStateMachines(operation *CommandRequest, shardID int) *CommandResponse {
	var value string
	var err Err
	switch operation.Op {
	case OpPut:
		err = kv.store.Put(shardID, operation.Key, operation.Value)
	case OpAppend:
		err = kv.store.Append(shardID, operation.Key, operation.Value)
	case OpGet:
		value, err = kv.store.Get(shardID, operation.Key)
	}
	return &CommandResponse{err, value}
}

func (kv *ShardKV) configureAction() {
	canPerformNextConfig := true
	kv.mu.RLock()
	for _, status := range kv.shardStatus {
		if status != Serving {
			canPerformNextConfig = false
			DPrintf("{Node %v}{Group %v} will not try to fetch latest configuration because shards status are %v when currentConfig is %v", kv.rf.Me(), kv.gid, kv.getShardStatus(), kv.currentConfig)
			break
		}
	}
	currentConfigNum := kv.currentConfig.Num
	kv.mu.RUnlock()
	if canPerformNextConfig {
		// Fetch the latest configuration information
		nextConfig := kv.sc.Query(currentConfigNum + 1)
		if nextConfig.Num == currentConfigNum+1 {
			DPrintf("{Node %v}{Group %v} fetches latest configuration %v when currentConfigNum is %v", kv.rf.Me(), kv.gid, nextConfig, currentConfigNum)
			// New configuration available; submit configuration update command and apply it to all servers in the group
			kv.Execute(NewConfigurationCommand(&nextConfig), &CommandResponse{})
		}
	}
}

func (kv *ShardKV) migrationAction() {
	kv.mu.RLock()
	gid2shardIDs := kv.getShardIDsByStatus(Pulling)
	var wg sync.WaitGroup
	for gid, shardIDs := range gid2shardIDs {
		DPrintf("{Node %v}{Group %v} starts a PullTask to get shards %v from group %v when config is %v", kv.rf.Me(), kv.gid, shardIDs, gid, kv.currentConfig)
		wg.Add(1)
		go func(servers []string, configNum int, shardIDs []int) {
			defer wg.Done()
			pullTaskRequest := &shardkvpb.ShardOperationRequest{
				ConfigNum: int32(configNum),
				ShardIds:  make([]int32, len(shardIDs)),
			}
			for i, sid := range shardIDs {
				pullTaskRequest.ShardIds[i] = int32(sid)
			}

			for _, server := range servers {
				var client ShardKVClient
				var conn *grpc.ClientConn
				if strings.Contains(server, ":") || strings.HasPrefix(server, "localhost") {
					var err error
					conn, err = grpc.Dial(server, grpc.WithTransportCredentials(insecure.NewCredentials()))
					if err != nil {
						continue
					}
					client = &gRPCShardKVClient{client: shardkvpb.NewShardKVServiceClient(conn)}
				} else {
					if kv.makeEnd != nil {
						client = &LabrpcShardKVClient{end: kv.makeEnd(server)}
					} else {
						continue
					}
				}

				resp, err := client.GetShardsData(context.Background(), pullTaskRequest)
				if conn != nil {
					conn.Close()
				}

				if err == nil && resp.Err == OK.String() {
					// Convert resp to ShardOperationResponse
					shards := make(map[int]map[string]string)
					for sid, data := range resp.Shards {
						shards[int(sid)] = data.Kv
					}
					lastOps := make(map[int64]OperationContext)
					for cid, opCtx := range resp.LastOperations {
						lastOps[cid] = OperationContext{
							MaxAppliedCommandId: opCtx.MaxAppliedCommandId,
							LastResponse: &CommandResponse{
								Err:   stringToErr(opCtx.LastResponse.Err),
								Value: opCtx.LastResponse.Value,
							},
						}
					}
					pullTaskResponse := ShardOperationResponse{
						Err:            OK,
						ConfigNum:      int(resp.ConfigNum),
						Shards:         shards,
						LastOperations: lastOps,
					}
					DPrintf("{Node %v}{Group %v} gets a PullTaskResponse %v and tries to commit it when currentConfigNum is %v", kv.rf.Me(), kv.gid, pullTaskResponse, configNum)
					kv.Execute(NewInsertShardsCommand(&pullTaskResponse), &CommandResponse{})
					return
				}
			}
		}(kv.lastConfig.Groups[gid], kv.currentConfig.Num, shardIDs)
	}
	kv.mu.RUnlock()
	wg.Wait()
}

func (kv *ShardKV) gcAction() {
	kv.mu.RLock()
	gid2shardIDs := kv.getShardIDsByStatus(GCing)
	var wg sync.WaitGroup
	for gid, shardIDs := range gid2shardIDs {
		DPrintf("{Node %v}{Group %v} starts a GCTask to delete shards %v in group %v when config is %v", kv.rf.Me(), kv.gid, shardIDs, gid, kv.currentConfig)
		wg.Add(1)
		go func(servers []string, configNum int, shardIDs []int) {
			defer wg.Done()
			gcTaskRequest := &shardkvpb.ShardOperationRequest{
				ConfigNum: int32(configNum),
				ShardIds:  make([]int32, len(shardIDs)),
			}
			for i, sid := range shardIDs {
				gcTaskRequest.ShardIds[i] = int32(sid)
			}

			for _, server := range servers {
				var client ShardKVClient
				var conn *grpc.ClientConn
				if strings.Contains(server, ":") || strings.HasPrefix(server, "localhost") {
					var err error
					conn, err = grpc.Dial(server, grpc.WithTransportCredentials(insecure.NewCredentials()))
					if err != nil {
						continue
					}
					client = &gRPCShardKVClient{client: shardkvpb.NewShardKVServiceClient(conn)}
				} else {
					if kv.makeEnd != nil {
						client = &LabrpcShardKVClient{end: kv.makeEnd(server)}
					} else {
						continue
					}
				}

				resp, err := client.DeleteShardsData(context.Background(), gcTaskRequest)
				if conn != nil {
					conn.Close()
				}

				if err == nil && resp.Err == OK.String() {
					DPrintf("{Node %v}{Group %v} deletes shards %v in remote group successfully when currentConfigNum is %v", kv.rf.Me(), kv.gid, shardIDs, configNum)
					localGCTaskRequest := ShardOperationRequest{ConfigNum: configNum, ShardIDs: shardIDs}
					kv.Execute(NewDeleteShardsCommand(&localGCTaskRequest), &CommandResponse{})
					return
				}
			}
		}(kv.lastConfig.Groups[gid], kv.currentConfig.Num, shardIDs)
	}
	kv.mu.RUnlock()
	wg.Wait()
}

func stringToErr(s string) Err {
	switch s {
	case "OK":
		return OK
	case "ErrNoKey":
		return ErrNoKey
	case "ErrWrongGroup":
		return ErrWrongGroup
	case "ErrWrongLeader":
		return ErrWrongLeader
	case "ErrOutDated":
		return ErrOutDated
	case "ErrTimeout":
		return ErrTimeout
	case "ErrNotReady":
		return ErrNotReady
	}
	return ErrWrongLeader
}

func (kv *ShardKV) checkEntryInCurrentTermAction() {
	if !kv.rf.HasLogInCurrentTerm() {
		kv.Execute(NewEmptyEntryCommand(), &CommandResponse{})
	}
}

func (kv *ShardKV) Monitor(action func(), timeout time.Duration) {
	for kv.killed() == false {
		if _, isLeader := kv.rf.GetState(); isLeader {
			action()
		}
		time.Sleep(timeout)
	}
}

// servers[] contains the ports of the servers in this group.
//
// me is the index of the current server in servers[].
//
// the k/v server should store snapshots through the underlying Raft
// implementation, which should call persister.SaveStateAndSnapshot() to
// atomically save the Raft state along with the snapshot.
//
// the k/v server should snapshot when Raft's saved state exceeds
// maxRaftState bytes, in order to allow Raft to garbage-collect its
// log. if maxRaftState is -1, you don't need to snapshot.
//
// gid is this group's GID, for interacting with the shardctrler.
//
// pass ctrlers[] to shardctrler.MakeClerk() so you can send
// RPCs to the shardctrler.
//
// make_end(servername) turns a server name from a
// Config.Groups[gid][i] into a labrpc.ClientEnd on which you can
// send RPCs. You'll need this to send RPCs to other groups.
//
// look at client.go for examples of how to use ctrlers[]
// and make_end() to send RPCs to the group owning a specific shard.
//
// StartServer() must return quickly, so it should start goroutines
// for any long-running work.
func StartServer(peers []raft.RaftPeer, me int, persister *raft.Persister, maxRaftState int, gid int, ctrlers []string, makeEnd func(string) *labrpc.ClientEnd, dbPath string) *ShardKV {
	// call labgob.Register on structures you want
	// Go's RPC library to marshall/unmarshall.
	labgob.Register(Command{})
	labgob.Register(CommandRequest{})
	labgob.Register(shardctrler.Config{})
	labgob.Register(ShardOperationResponse{})
	labgob.Register(ShardOperationRequest{})

	applyCh := make(chan raft.ApplyMsg)

	kv := &ShardKV{
		dead:           0,
		rf:             raft.Make(peers, me, persister, applyCh),
		applyCh:        applyCh,
		gid:            gid,
		sc:             shardctrler.MakeClerkWithMakeEnd(ctrlers, makeEnd),
		lastApplied:    0,
		maxRaftState:   maxRaftState,
		currentConfig:  shardctrler.DefaultConfig(),
		lastConfig:     shardctrler.DefaultConfig(),
		store:          NewLevelDBShardStore(dbPath),
		lastOperations: make(map[int64]OperationContext),
		notifyChans:    make(map[int]chan *CommandResponse),
		makeEnd:        makeEnd,
	}
	kv.restoreSnapshot(persister.ReadSnapshot())
	// start applier goroutine to apply committed logs to stateMachine
	go kv.applier()
	// start configuration monitor goroutine to fetch latest configuration
	go kv.Monitor(kv.configureAction, ConfigureMonitorTimeout)
	// start migration monitor goroutine to pull related shards
	go kv.Monitor(kv.migrationAction, MigrationMonitorTimeout)
	// start gc monitor goroutine to delete useless shards in remote groups
	go kv.Monitor(kv.gcAction, GCMonitorTimeout)
	// start entry-in-currentTerm monitor goroutine to advance commitIndex by appending empty entries in current term periodically to avoid live locks
	go kv.Monitor(kv.checkEntryInCurrentTermAction, EmptyEntryDetectorTimeout)

	DPrintf("{Node %v}{Group %v} has started", kv.rf.Me(), kv.gid)
	return kv
}
