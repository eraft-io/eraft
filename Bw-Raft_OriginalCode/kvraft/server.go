package kvraft

import (
	"../labgob"
	"../labrpc"
	"log"
	"../raft"
	"sync"
	"sync/atomic"
	"time"
)

const Debug = 0

func DPrintf(format string, a ...interface{}) (n int, err error) {
	if Debug > 0 {
		log.Printf(format, a...)
	}
	return
}


type Op struct {
	// Your definitions here.
	// Field names must start with capital letters,
	// otherwise RPC will break.
	Option string
	Key string 
	Value string
	Id int64
	Seq int64
}

type KVServer struct {
	mu      sync.Mutex
	me      int
	rf      *raft.Raft
	applyCh chan raft.ApplyMsg
	dead    int32 // set by Kill()
	maxraftstate int // snapshot if log grows this big

	// Your definitions here.
	Seq map[int64]int64
	db map[string]string
	chMap map[int]chan Op
}


func (kv *KVServer) Get(args *GetArgs, reply *GetReply) {
	_ , isLeader := kv.rf.GetState()
	reply.IsLeader = false;
	if !isLeader{
		return
	} 
	oringalOp := Op{"Get", args.Key,"" , 0, 0}
	index, _, isLeader  := kv.rf.Start(oringalOp)
	if !isLeader {
		return
	}
	ch := kv.getChan(index)
	op := kv.replyNotify(ch)
	
	if args.Key == op.Key{
		reply.IsLeader = true
		kv.mu.Lock()
		reply.Value = kv.db[args.Key]
		kv.mu.Unlock()
		return
	} 
	
}

func (kv *KVServer) PutAppend(args *PutAppendArgs, reply *PutAppendReply) {
	// Your code here.
	_ , isLeader := kv.rf.GetState()
	reply.IsLeader = false;
	if !isLeader{
		return
	}
	oringalOp := Op{args.Op, args.Key,args.Value, args.Id, args.Seq}
	index, _, isLeader := kv.rf.Start(oringalOp)
	if !isLeader {
		return
	}
	ch := kv.getChan(index)
	op := kv.replyNotify(ch)
	
	if kv.equal(oringalOp , op){
		reply.IsLeader = true
		return
	} 

}
func (kv *KVServer) equal(a Op, b Op) bool  {
	return (a.Option == b.Option && a.Key == b.Key &&a.Value == b.Value) 
}


func (kv *KVServer) getChan(index int) chan Op {
	kv.mu.Lock()
	defer kv.mu.Unlock()
	_, ok := kv.chMap[index]
	if !ok{
		kv.chMap[index] = make(chan Op,1)
	}
	return kv.chMap[index]
}


func (kv *KVServer) replyNotify(ch chan Op) Op {
	select{
	case op := <- ch:
		return op
	case <- time.After(time.Second):
		return Op{}
	}
}

//
// the tester calls Kill() when a KVServer instance won't
// be needed again. for your convenience, we supply
// code to set rf.dead (without needing a lock),
// and a killed() method to test rf.dead in
// long-running loops. you can also add your own
// code to Kill(). you're not required to do anything
// about this, but it may be convenient (for example)
// to suppress debug output from a Kill()ed instance.
//
func (kv *KVServer) Kill() {
	atomic.StoreInt32(&kv.dead, 1)
	kv.rf.Kill()
	// Your code here, if desired.
}

func (kv *KVServer) killed() bool {
	z := atomic.LoadInt32(&kv.dead)
	return z == 1
}

//
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
//
func StartKVServer(servers []*labrpc.ClientEnd, me int,
	persister *raft.Persister, maxraftstate int) *KVServer {
	// call labgob.Register on structures you want
	// Go's RPC library to marshall/unmarshall.
	labgob.Register(Op{})

	kv := new(KVServer)
	kv.me = me
	kv.maxraftstate = maxraftstate

	// You may need initialization code here.

	kv.applyCh = make(chan raft.ApplyMsg)
	kv.rf = raft.Make(servers, me, persister, kv.applyCh)

	// You may need initialization code here.
	kv.db = make(map[string]string)
	kv.chMap = make(map[int]chan Op)
	kv.Seq = make(map[int64]int64)

	go func ()  {
		for applyMsg := range kv.applyCh {
			// .(Op) is used to convert interface type to Op type  
			op := applyMsg.Command.(Op) 
			kv.mu.Lock()
			MaxSeq, find := kv.Seq[op.Id]
			if (!find || MaxSeq < op.Seq){
				switch op.Option {
					case "Append":  kv.db[op.Key] += op.Value;
					case "Put":		kv.db[op.Key] = op.Value;
				}
				kv.Seq[op.Id] = op.Seq
			}
			kv.mu.Unlock()
			
			index :=  applyMsg.CommandIndex
			ch := kv.getChan(index)
			ch <- op
		}
	}()	
	return kv
}
