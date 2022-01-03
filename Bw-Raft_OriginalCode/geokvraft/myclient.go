package main

import (
	"flag"
	"fmt"
	"log"
	"sync/atomic"
	"time"

	//"strconv"
	"strings"

	"golang.org/x/net/context"
	"google.golang.org/grpc"

	//"google.golang.org/grpc/reflection"
	crand "crypto/rand"
	"math/big"
	"math/rand"
	"strconv"

	KV "../grpc/mykv"
	//Per "../persister"
	//"math/rand"
)

type Clerk struct {
	servers []string
	// You will have to modify this struct.
	leaderId int
	id       int64
	seq      int64
}

func makeSeed() int64 {
	max := big.NewInt(int64(1) << 62)
	bigx, _ := crand.Int(crand.Reader, max)
	x := bigx.Int64()
	return x
}

func MakeClerk(servers []string) *Clerk {
	ck := new(Clerk)
	ck.servers = servers
	ck.id = makeSeed()
	ck.seq = 0
	// You'll have to add code here.
	return ck
}

func (ck *Clerk) Get(key string) string {
	args := &KV.GetArgs{Key: key}
	id := rand.Intn(len(ck.servers)+10) % len(ck.servers)
	for {
		reply, ok := ck.getValue(ck.servers[id], args)
		if ok {
			//fmt.Println(id)
			return reply.Value
		} else {
			fmt.Println("can not connect ", ck.servers[id], "or it's not leader")
		}
		id = rand.Intn(len(ck.servers)+10) % len(ck.servers)
	}
}

func (ck *Clerk) getValue(address string, args *KV.GetArgs) (*KV.GetReply, bool) {
	// Initialize Client
	conn, err := grpc.Dial(address, grpc.WithInsecure()) //,grpc.WithBlock())
	if err != nil {
		log.Printf("did not connect: %v", err)
		return nil, false
	}
	defer conn.Close()
	client := KV.NewKVClient(conn)
	ctx, cancel := context.WithTimeout(context.Background(), time.Second*5)
	defer cancel()
	reply, err := client.Get(ctx, args)
	if err != nil {
		return nil, false
		log.Printf(" getValue could not greet: %v", err)
	}
	return reply, true
}

func (ck *Clerk) Put(key string, value string) bool {
	// You will have to modify this function.
	args := &KV.PutAppendArgs{Key: key, Value: value, Op: "Put", Id: ck.id, Seq: ck.seq}
	id := ck.leaderId
	for {
		//fmt.Println(id)
		reply, ok := ck.putAppendValue(ck.servers[id], args)
		//fmt.Println(ok)

		if ok && reply.IsLeader {
			ck.leaderId = id
			return true
		} else {
			fmt.Println(ok, "can not connect ", ck.servers[id], "or it's not leader")
		}
		id = (id + 1) % len(ck.servers)
	}
}

func (ck *Clerk) Append(key string, value string) bool {
	// You will have to modify this function.
	args := &KV.PutAppendArgs{Key: key, Value: value, Op: "Append", Id: ck.id, Seq: ck.seq}
	id := ck.leaderId
	for {
		reply, ok := ck.putAppendValue(ck.servers[id], args)
		if ok && reply.IsLeader {
			ck.leaderId = id
			return true
		}
		id = (id + 1) % len(ck.servers)
	}
}

func (ck *Clerk) putAppendValue(address string, args *KV.PutAppendArgs) (*KV.PutAppendReply, bool) {
	// Initialize Client
	conn, err := grpc.Dial(address, grpc.WithInsecure()) //,grpc.WithBlock())
	if err != nil {
		return nil, false
		log.Printf(" did not connect: %v", err)
	}
	defer conn.Close()
	client := KV.NewKVClient(conn)
	ctx, cancel := context.WithTimeout(context.Background(), time.Second*5)
	defer cancel()
	reply, err := client.PutAppend(ctx, args)
	if err != nil {
		return nil, false
		log.Printf("  putAppendValue could not greet: %v", err)
	}
	return reply, true
}

var count int32 = 0

func Wirterequest(num int, servers []string) {
	ck := Clerk{}
	ck.servers = make([]string, len(servers))

	for i := 0; i < len(servers); i++ {
		ck.servers[i] = servers[i] + "1"
	}
	ck.Put("key", "value")
	for i := 0; i < num; i++ {
		key := "key" + strconv.Itoa(rand.Intn(100000))
		value := "value" + strconv.Itoa(rand.Intn(100000))
		ck.Put(key, value)
		atomic.AddInt32(&count, 1)
	}
}

func Readrequest(num int, servers []string) {
	ck := Clerk{}
	ck.servers = make([]string, len(servers))

	for i := 0; i < len(servers); i++ {
		ck.servers[i] = servers[i] + "1"
	}

	for i := 0; i < num; i++ {
		ck.Get("key")
		atomic.AddInt32(&count, 1)
	}
}

func ReadLatency(num int, servers []string) {
	ck := Clerk{}
	ck.servers = make([]string, len(servers))

	for i := 0; i < len(servers); i++ {
		ck.servers[i] = servers[i] + "1"
	}
	ck.Put("key", "value")
	begin := time.Now().UnixNano()
	for i := 0; i < num; i++ {
		ck.Get("key")
		atomic.AddInt32(&count, 1)
	}
	end := time.Now().UnixNano()
	fmt.Println("Average read Latency: ", (end-begin)/int64(num)/1e6, " ms")
}

func WriteLatency(num int, servers []string) {
	ck := Clerk{}
	ck.servers = make([]string, len(servers))

	for i := 0; i < len(servers); i++ {
		ck.servers[i] = servers[i] + "1"
	}
	begin := time.Now().UnixNano()
	for i := 0; i < num; i++ {
		key := "key" + strconv.Itoa(rand.Intn(100000))
		value := "value" + strconv.Itoa(rand.Intn(100000))
		ck.Put(key, value)
		atomic.AddInt32(&count, 1)
	}
	end := time.Now().UnixNano()
	fmt.Println("Average write Latency: ", (end-begin)/int64(num)/1e6, " ms")
}

func main() {

	var ser = flag.String("servers", "", "Input Your follower")
	var mode = flag.String("mode", "", "Input Your follower")
	var cnums = flag.String("cnums", "", "Input Your follower")
	var onums = flag.String("onums", "", "Input Your follower")
	flag.Parse()
	servers := strings.Split(*ser, ",")
	clientNumm, _ := strconv.Atoi(*cnums)
	optionNumm, _ := strconv.Atoi(*onums)

	if clientNumm == 0 {
		fmt.Println("##########################################")
		fmt.Println("### Don't forget input -cnum's value ! ###")
		fmt.Println("##########################################")
		return
	}
	if optionNumm == 0 {
		fmt.Println("##########################################")
		fmt.Println("### Don't forget input -onumm's value ! ###")
		fmt.Println("##########################################")
		return
	}

	if *mode == "write" {
		for i := 0; i < clientNumm; i++ {
			go Wirterequest(optionNumm, servers)
		}
	} else if *mode == "read" {
		for i := 0; i < clientNumm; i++ {
			go Readrequest(optionNumm, servers)
		}
	} else if *mode == "readlatency" {
		for i := 0; i < clientNumm; i++ {
			go ReadLatency(optionNumm, servers)
		}
	} else if *mode == "writelatency" {
		for i := 0; i < clientNumm; i++ {
			go WriteLatency(optionNumm, servers)
		}
	} else {
		fmt.Println("### Wrong Mode ! ###")
		return
	}
	fmt.Println("count")
	time.Sleep(time.Second * 3)
	fmt.Println(count / 3)
	//	return

	time.Sleep(time.Second * 1200)

}
