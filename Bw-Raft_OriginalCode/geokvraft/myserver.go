package main

import (
	"flag"
	"fmt"
	"log"
	"math/rand"
	"net"
	"strconv"
	"strings"
	"sync"
	"time"

	config "../config"
	"../georaft"
	KV "../grpc/mykv"
	Per "../persister"
	"golang.org/x/net/context"
	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"
)

type KVServer struct {
	mu      sync.Mutex
	me      int
	rf      *georaft.GeoRaft
	applyCh chan int
	//applyCh chan raft.ApplyMsg
	dead         int32 // set by Kill()
	maxraftstate int   // snapshot if log grows this big

	// Your definitions here.
	Seq     map[int64]int64
	db      map[string]string
	chMap   map[int]chan config.Op
	persist *Per.Persister
	delay   int
}

func (kv *KVServer) PutAppend(ctx context.Context, args *KV.PutAppendArgs) (*KV.PutAppendReply, error) {
	// Your code here.
	time.Sleep(time.Millisecond * time.Duration(kv.delay+rand.Intn(25)))

	//time.Sleep(time.Second)
	reply := &KV.PutAppendReply{}
	_, reply.IsLeader = kv.rf.GetState()
	//reply.IsLeader = false;
	if !reply.IsLeader {
		return reply, nil
	}
	oringalOp := config.Op{args.Op, args.Key, args.Value, args.Id, args.Seq}
	index, _, isLeader := kv.rf.Start(oringalOp)
	if !isLeader {
		fmt.Println("Leader Changed !")
		reply.IsLeader = false
		return reply, nil
	}

	apply := <-kv.applyCh
	//fmt.Println(apply)

	fmt.Println(index)
	if apply == 1 {

	}
	return reply, nil

}

func (kv *KVServer) Get(ctx context.Context, args *KV.GetArgs) (*KV.GetReply, error) {
	time.Sleep(time.Millisecond * time.Duration(kv.delay+rand.Intn(25)))

	reply := &KV.GetReply{}
	_, reply.IsLeader = kv.rf.GetState()
	//reply.IsLeader = false;
	if !reply.IsLeader {
		return reply, nil
	}

	//fmt.Println()

	//oringalOp := config.Op{"Get", args.Key, "", 0, 0}
	//	_, _, isLeader := kv.rf.Start(oringalOp)
	//if !isLeader {
	//	return reply, nil
	//}
	//fmt.Println(index)

	reply.IsLeader = true
	//kv.mu.Lock()
	//fmt.Println("Asdsada")
	reply.Value = string(kv.persist.Get(args.Key))
	//fmt.Println("Asdsada")

	//kv.mu.Unlock()
	return reply, nil
}

func (kv *KVServer) equal(a config.Op, b config.Op) bool {
	return (a.Option == b.Option && a.Key == b.Key && a.Value == b.Value)
}

func (kv *KVServer) RegisterServer(address string) {
	// Register Server
	for {

		lis, err := net.Listen("tcp", address)
		fmt.Println(address)
		if err != nil {
			log.Fatalf("failed to listen: %v", err)
		}
		s := grpc.NewServer()
		KV.RegisterKVServer(s, kv)
		// Register reflection service on gRPC server.
		reflection.Register(s)
		if err := s.Serve(lis); err != nil {
			fmt.Println("failed to serve: %v", err)
		}

	}

}

func main() {

	var add = flag.String("address", "", "Input Your address")
	var mems = flag.String("members", "", "Input Your follower")
	var sec = flag.String("secretaries", "", "Input Your secretary")
	var ob = flag.String("observers", "", "Input Your secretary")
	var secm = flag.String("secmembers", "", "Input Your secretary")
	var delays = flag.String("delay", "", "Input Your follower")

	flag.Parse()

	server := KVServer{}

	// Local address
	address := *add

	persist := &Per.Persister{}
	persist.Init("../db/" + address)

	//for i := 0; i <= int (address[ len(address) - 1] - '0'); i++{
	server.applyCh = make(chan int, 1)
	delay, _ := strconv.Atoi(*delays)
	server.delay = delay
	fmt.Println("delay", server.delay)

	server.persist = persist

	// Members's address
	members := strings.Split(*mems, ",")
	secretaries := strings.Split(*sec, ",")
	observers := strings.Split(*ob, ",")
	secmembers := strings.Split(*secm, ",")
	/* if secmembers[0] == "" {
		fmt.Println(secmembers)
		secmembers = nil
	} */

	fmt.Println(address, members, secretaries, secretaries == nil)

	go server.RegisterServer(address + "1")
	server.rf = georaft.MakeGeoRaft(address, members, secretaries, secmembers,
		observers, persist, &server.mu, server.applyCh, delay)

	time.Sleep(time.Second * 1200)

}
