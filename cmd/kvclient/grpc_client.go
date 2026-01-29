package main

import (
	"flag"
	"fmt"
	"math/rand"
	"strings"
	"time"

	"github.com/eraft-io/eraft/kvraft"
	"github.com/eraft-io/eraft/labrpc"
	"github.com/eraft-io/eraft/logger"
)

var serverAddrs = flag.String("servers", "127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090", "server node address")
var key = flag.String("key", "", "operation key")
var val = flag.String("val", "", "operation value")
var op = flag.String("op", "", "operation (put/get/bench)")
var benchN = flag.Int("n", 1000, "number of operations in bench mode")

func init() {
	rand.Seed(time.Now().UnixNano())
}

func randomString(prefix string, maxID int) string {
	return fmt.Sprintf("%s-%d", prefix, rand.Intn(maxID))
}

func main() {
	flag.Parse()

	ServerAddrs := strings.Split(*serverAddrs, ",")
	serverClients := []*labrpc.ClientEnd{}
	for i, serverAddr := range ServerAddrs {
		client := &labrpc.ClientEnd{}
		client.GrpcClient = labrpc.MakeGrpcClientEnd(uint64(i), serverAddr)
		serverClients = append(serverClients, client)
	}
	kvClerk := kvraft.MakeClerk(serverClients)

	switch *op {
	case "put":
		kvClerk.Put(*key, *val)
		logger.ELogger().Sugar().Infof("put key %s value %s to server success", *key, *val)

	case "get":
		gotVal := kvClerk.Get(*key)
		logger.ELogger().Sugar().Infof("get value %v from server", gotVal)

	case "bench":
		logger.ELogger().Sugar().Infof("starting benchmark: %d PUT operations", *benchN)
		for i := 0; i < *benchN; i++ {
			randKey := randomString("key", 1000)
			randVal := randomString("val", 1000)
			kvClerk.Put(randKey, randVal)
		}
		logger.ELogger().Sugar().Infof("benchmark finished: %d PUT operations completed", *benchN)

	default:
		logger.ELogger().Sugar().Errorf("unknown operation: %s, expected put/get/bench", *op)
	}
}
