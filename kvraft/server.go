package kvraft

import (
	"bytes"
	"fmt"
	"sync"
	"sync/atomic"
	"time"

	"github.com/eraft-io/eraft/labgob"
	"github.com/eraft-io/eraft/raft"
	"github.com/eraft-io/eraft/storage"
)

type KVStateMachine interface {
	Get(key string) (string, Err)
	Put(key, value string) Err
	Append(key, value string) Err
	Close()
	Size() int64
}

type RocksDBKV struct {
	storage.Storage
}

func NewRocksDBKV(path string) *RocksDBKV {
	db, err := storage.NewRocksDBStorage(path, nil)
	if err != nil {
		panic(err)
	}
	return &RocksDBKV{Storage: db}
}

func (rk *RocksDBKV) Get(key string) (string, Err) {
	value, err := rk.Storage.Get([]byte(key))
	if err != nil {
		return "", ErrTimeout
	}
	if value == nil {
		return "", ErrNoKey
	}
	return string(value), OK
}

func (rk *RocksDBKV) Put(key, value string) Err {
	err := rk.Storage.Put([]byte(key), []byte(value))
	if err != nil {
		return ErrTimeout
	}
	return OK
}

func (rk *RocksDBKV) Append(key, value string) Err {
	oldValue, err := rk.Storage.Get([]byte(key))
	if err != nil {
		return ErrTimeout
	}
	newValue := append(oldValue, []byte(value)...)
	err = rk.Storage.Put([]byte(key), newValue)
	if err != nil {
		return ErrTimeout
	}
	return OK
}

func (rk *RocksDBKV) Close() {
	rk.Storage.Close()
}

func (rk *RocksDBKV) Size() int64 {
	return rk.Storage.Size()
}

type KVServer struct {
	mu      sync.RWMutex
	dead    int32
	rf      *raft.Raft
	applyCh chan raft.ApplyMsg

	maxRaftState int // snapshot if log grows this big
	lastApplied  int // record the lastApplied to prevent stateMachine from rollback

	stateMachine   KVStateMachine                // KV stateMachine
	lastOperations map[int64]OperationContext    // determine whether log is duplicated by recording the last commandId and response corresponding to the clientId
	notifyChans    map[int]chan *CommandResponse // notify client goroutine by applier goroutine to response
}

func (kv *KVServer) Command(request *CommandRequest, response *CommandResponse) {
	defer DPrintf("{Node %v} processes CommandRequest %v with CommandResponse %v", kv.rf.Me(), request, response)
	// return result directly without raft layer's participation if request is duplicated
	kv.mu.RLock()
	if request.Op != OpGet && kv.isDuplicateRequest(request.ClientId, request.CommandId) {
		lastResponse := kv.lastOperations[request.ClientId].LastResponse
		response.Value, response.Err = lastResponse.Value, lastResponse.Err
		kv.mu.RUnlock()
		return
	}
	kv.mu.RUnlock()
	// do not hold lock to improve throughput
	// when KVServer holds the lock to take snapshot, underlying raft can still commit raft logs
	w := new(bytes.Buffer)
	e := labgob.NewEncoder(w)
	e.Encode(Command{request})
	index, _, isLeader := kv.rf.Start(w.Bytes())
	if !isLeader {
		response.Err = ErrWrongLeader
		return
	}
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

// each RPC imply that the client has seen the reply for its previous RPC
// therefore, we only need to determine whether the latest commandId of a clientId meets the criteria
func (kv *KVServer) isDuplicateRequest(clientId int64, requestId int64) bool {
	operationContext, ok := kv.lastOperations[clientId]
	return ok && requestId <= operationContext.MaxAppliedCommandId
}

// the tester calls Kill() when a KVServer instance won't
// be needed again. for your convenience, we supply
// code to set rf.dead (without needing a lock),
// and a killed() method to test rf.dead in
// long-running loops. you can also add your own
// code to Kill(). you're not required to do anything
// about this, but it may be convenient (for example)
// to suppress debug output from a Kill()ed instance.
func (kv *KVServer) Kill() {
	DPrintf("{Node %v} has been killed", kv.rf.Me())
	atomic.StoreInt32(&kv.dead, 1)
	kv.rf.Kill()
	kv.stateMachine.Close()
}

func (kv *KVServer) killed() bool {
	return atomic.LoadInt32(&kv.dead) == 1
}

func (kv *KVServer) Raft() *raft.Raft {
	return kv.rf
}

func (kv *KVServer) GetStatus() (int, string, int, int, int, int64) {
	id, state, term, applied, commit := kv.rf.GetStatus()
	return id, state, term, applied, commit, kv.stateMachine.Size() + int64(kv.rf.GetRaftStateSize())
}

// a dedicated applier goroutine to apply committed entries to stateMachine, take snapshot and apply snapshot from raft
func (kv *KVServer) applier() {
	for kv.killed() == false {
		select {
		case message := <-kv.applyCh:
			DPrintf("{Node %v} tries to apply message %v", kv.rf.Me(), message)
			if message.CommandValid {
				kv.mu.Lock()
				if message.CommandIndex <= kv.lastApplied {
					DPrintf("{Node %v} discards outdated message %v because a newer snapshot which lastApplied is %v has been restored", kv.rf.Me(), message, kv.lastApplied)
					kv.mu.Unlock()
					continue
				}
				kv.lastApplied = message.CommandIndex

				var response *CommandResponse
				var command Command
				commandBytes, ok := message.Command.([]byte)
				if !ok {
					command = message.Command.(Command)
				} else {
					r := bytes.NewBuffer(commandBytes)
					d := labgob.NewDecoder(r)
					d.Decode(&command)
				}

				if command.Op != OpGet && kv.isDuplicateRequest(command.ClientId, command.CommandId) {
					DPrintf("{Node %v} doesn't apply duplicated message %v to stateMachine because maxAppliedCommandId is %v for client %v", kv.rf.Me(), message, kv.lastOperations[command.ClientId], command.ClientId)
					response = kv.lastOperations[command.ClientId].LastResponse
				} else {
					response = kv.applyLogToStateMachine(command)
					if command.Op != OpGet {
						kv.lastOperations[command.ClientId] = OperationContext{command.CommandId, response}
					}
				}

				// only notify related channel for currentTerm's log when node is leader
				if currentTerm, isLeader := kv.rf.GetState(); isLeader && message.CommandTerm == currentTerm {
					ch := kv.getNotifyChan(message.CommandIndex)
					ch <- response
				}

				// Determine if a snapshot is needed by checking if the size of the state data, including logs, is greater than kv.maxRaftState
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

func (kv *KVServer) needSnapshot() bool {
	return kv.maxRaftState != -1 && kv.rf.GetRaftStateSize() >= kv.maxRaftState
}

func (kv *KVServer) takeSnapshot(index int) {
	// Take a snapshot: state machine data, last operation, and the index of the last operation
	kvMap := make(map[string]string)
	if rk, ok := kv.stateMachine.(*RocksDBKV); ok {
		iter := rk.Storage.NewIterator(nil)
		for iter.Valid() {
			kvMap[string(iter.Key())] = string(iter.Value())
			iter.Next()
		}
		iter.Close()
	}

	w := new(bytes.Buffer)
	e := labgob.NewEncoder(w)
	e.Encode(kvMap)
	e.Encode(kv.lastOperations)
	kv.rf.Snapshot(index, w.Bytes())
}

func (kv *KVServer) restoreSnapshot(snapshot []byte) {
	if snapshot == nil || len(snapshot) == 0 {
		return
	}
	r := bytes.NewBuffer(snapshot)
	d := labgob.NewDecoder(r)
	var kvMap map[string]string
	var lastOperations map[int64]OperationContext
	if d.Decode(&kvMap) != nil ||
		d.Decode(&lastOperations) != nil {
		DPrintf("{Node %v} restores snapshot failed", kv.rf.Me())
		return
	}
	if rk, ok := kv.stateMachine.(*RocksDBKV); ok {
		for k, v := range kvMap {
			rk.Put(k, v)
		}
	}
	kv.lastOperations = lastOperations
}

func (kv *KVServer) getNotifyChan(index int) chan *CommandResponse {
	if _, ok := kv.notifyChans[index]; !ok {
		kv.notifyChans[index] = make(chan *CommandResponse, 1)
	}
	return kv.notifyChans[index]
}

func (kv *KVServer) removeOutdatedNotifyChan(index int) {
	delete(kv.notifyChans, index)
}

func (kv *KVServer) applyLogToStateMachine(command Command) *CommandResponse {
	var value string
	var err Err
	switch command.Op {
	case OpPut:
		err = kv.stateMachine.Put(command.Key, command.Value)
	case OpAppend:
		err = kv.stateMachine.Append(command.Key, command.Value)
	case OpGet:
		value, err = kv.stateMachine.Get(command.Key)
	}
	return &CommandResponse{err, value}
}

// servers[] contains the ports of the set of
// servers that will cooperate via Raft to
// form the fault-tolerant key/value service.
// me is the index of the current server in servers[].
// the k/v server should store snapshots through the underlying Raft
// implementation, which should call persister.SaveStateAndSnapshot() to
// atomically save the Raft state along with the snapshot.
// the k/v server should snapshot when Raft's saved state exceeds maxraftstate bytes,
// in order to allow Raft to garbage-collect its log. if maxraftstate is -1,
// you don't need to snapshot.
// StartKVServer() must return quickly, so it should start goroutines
// for any long-running work.
func StartKVServer(peers []raft.RaftPeer, me int, persister *raft.Persister, maxraftstate int, dbPath string) *KVServer {
	// call labgob.Register on structures you want
	// Go's RPC library to marshall/unmarshall.
	labgob.Register(Command{})
	applyCh := make(chan raft.ApplyMsg)

	kv := &KVServer{
		maxRaftState:   maxraftstate,
		applyCh:        applyCh,
		dead:           0,
		lastApplied:    0,
		rf:             raft.Make(peers, me, persister, applyCh),
		stateMachine:   NewRocksDBKV(dbPath),
		lastOperations: make(map[int64]OperationContext),
		notifyChans:    make(map[int]chan *CommandResponse),
	}
	// Restore state from snapshot, including KV data in the state machine and the last operation
	kv.restoreSnapshot(persister.ReadSnapshot())
	// start applier goroutine to apply committed logs to stateMachine
	go kv.applier()

	DPrintf("{Node %v} has started", kv.rf.Me())
	return kv
}
