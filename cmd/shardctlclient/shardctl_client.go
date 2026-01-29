package main

import (
	"flag"
	"fmt"
	"math/rand"
	"strings"
	"time"

	"github.com/eraft-io/eraft/labrpc"
	"github.com/eraft-io/eraft/shardctrler"
)

var shardctlAddrs = flag.String("servers", "127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090", "shardctl server node address")
var op = flag.String("op", "", "operation (query, join, leave, move)")
var gid = flag.Int("gid", 0, "group id")
var slot = flag.Int("slot", 0, "shard slot")
var groupServers = flag.String("gservers", "127.0.0.1:6088,127.0.0.1:6089,127.0.0.1:6090", "group servers (comma separated)")

func init() {
	rand.Seed(time.Now().UnixNano())
}

func randomString(prefix string, maxID int) string {
	return fmt.Sprintf("%s-%d", prefix, rand.Intn(maxID))
}

func main() {
	flag.Parse()

	ServerAddrs := strings.Split(*shardctlAddrs, ",")
	serverClients := []*labrpc.ClientEnd{}
	for i, serverAddr := range ServerAddrs {
		client := &labrpc.ClientEnd{}
		client.GrpcClient = labrpc.MakeGrpcClientEnd(uint64(i), serverAddr)
		serverClients = append(serverClients, client)
	}
	ctlClerk := shardctrler.MakeClerk(serverClients)
	switch *op {
	case "join":
		ctlClerk.Join(map[int][]string{*gid: strings.Split(*groupServers, ",")})
		fmt.Printf("server group %s join to gid %d success!\n", *groupServers, *gid)
	case "leave":
		ctlClerk.Leave([]int{*gid})
		fmt.Printf("leave gid %d success!\n", *gid)
	case "move":
		ctlClerk.Move(*slot, *gid)
		fmt.Printf("move slot %d to gid %d success!\n", *slot, *gid)
	case "query":
		config := ctlClerk.Query(-1)
		fmt.Printf("%v\n", config)
	default:
		fmt.Printf("unknown operation %s\n", *op)
	}

}
