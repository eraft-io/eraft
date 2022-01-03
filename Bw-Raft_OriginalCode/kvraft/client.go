package kvraft

import "../labrpc"
import "crypto/rand"
import "math/big"
import "fmt"

type Clerk struct {
	servers []*labrpc.ClientEnd
	// You will have to modify this struct.
	leaderId int
	id int64
	seq int64
}

func nrand() int64 {
	max := big.NewInt(int64(1) << 62)
	bigx, _ := rand.Int(rand.Reader, max)
	x := bigx.Int64()
	return x
}


func MakeClerk(servers []*labrpc.ClientEnd) *Clerk {
	ck := new(Clerk)
	ck.servers = servers
	ck.id = makeSeed()
	ck.seq = 0
	// You'll have to add code here.
	return ck
}

//
// fetch the current value for a key.
// returns "" if the key does not exist.
// keeps trying forever in the face of all other errors.
//
// you can send an RPC with code like this:
// ok := ck.servers[i].Call("KVServer.Get", &args, &reply)
//
// the types of args and reply (including whether they are pointers)
// must match the declared types of the RPC handler function's
// arguments. and reply must be passed as a pointer.
//
func (ck *Clerk) Get(key string) string {
	
	id := ck.leaderId
	args := GetArgs{key}
	reply  := GetReply{}
	for {
		ok := ck.servers[id].Call("KVServer.Get",&args, &reply )	
		if (ok && reply.IsLeader){
			ck.leaderId = id;
			//fmt.Println("get value ",  reply.Value)
			return reply.Value;
		}
//		fmt.Println("Wrong leader !")

		id = (id + 1) % len(ck.servers) 
	}
	
	//return reply.Value
}

//
// shared by Put and Append.
//
// you can send an RPC with code like this:
// ok := ck.servers[i].Call("KVServer.PutAppend", &args, &reply)
//
// the types of args and reply (including whether they are pointers)
// must match the declared types of the RPC handler function's
// arguments. and reply must be passed as a pointer.
//
func (ck *Clerk) PutAppend(key string, value string, op string) {
	// You will have to modify this function.
	args := PutAppendArgs{key,value,op, ck.id,ck.seq}
	ck.seq++
	id := ck.leaderId
	for {
		reply := PutAppendReply{}
		ok := ck.servers[id].Call("KVServer.PutAppend", &args, &reply)
		if (ok && reply.IsLeader){
			ck.leaderId = id;
		//	fmt.Println("putappend", key , value)
			return 
		}
		fmt.Print()
		id = (id + 1) % len(ck.servers)
	} 
}

func (ck *Clerk) Put(key string, value string) {
	ck.PutAppend(key, value, "Put")
}
func (ck *Clerk) Append(key string, value string) {
	ck.PutAppend(key, value, "Append")
}
