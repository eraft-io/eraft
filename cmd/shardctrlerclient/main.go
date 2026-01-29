package main

import (
	"flag"
	"fmt"
	"strconv"
	"strings"

	"github.com/eraft-io/eraft/shardctrler"
)

func main() {
	cluster := flag.String("cluster", "127.0.0.1:50051,127.0.0.1:50052,127.0.0.1:50053", "cluster addresses")
	flag.Parse()

	addrs := strings.Split(*cluster, ",")
	ck := shardctrler.MakeClerk(addrs)

	if len(flag.Args()) < 1 {
		usage()
		return
	}

	cmd := flag.Arg(0)
	switch cmd {
	case "join":
		if len(flag.Args()) < 2 {
			fmt.Println("Usage: join <gid>=<server1>,<server2>...")
			return
		}
		groups := make(map[int][]string)
		for _, arg := range flag.Args()[1:] {
			parts := strings.Split(arg, "=")
			if len(parts) != 2 {
				continue
			}
			gid, _ := strconv.Atoi(parts[0])
			servers := strings.Split(parts[1], ",")
			groups[gid] = servers
		}
		ck.Join(groups)
		fmt.Println("Join OK")
	case "leave":
		if len(flag.Args()) < 2 {
			fmt.Println("Usage: leave <gid1> <gid2>...")
			return
		}
		var gids []int
		for _, arg := range flag.Args()[1:] {
			gid, _ := strconv.Atoi(arg)
			gids = append(gids, gid)
		}
		ck.Leave(gids)
		fmt.Println("Leave OK")
	case "move":
		if len(flag.Args()) < 3 {
			fmt.Println("Usage: move <shard> <gid>")
			return
		}
		shard, _ := strconv.Atoi(flag.Arg(1))
		gid, _ := strconv.Atoi(flag.Arg(2))
		ck.Move(shard, gid)
		fmt.Println("Move OK")
	case "query":
		num := -1
		if len(flag.Args()) >= 2 {
			num, _ = strconv.Atoi(flag.Arg(1))
		}
		config := ck.Query(num)
		fmt.Printf("Config #%d\n", config.Num)
		fmt.Printf("  Shards: %v\n", config.Shards)
		fmt.Printf("  Groups:\n")
		for gid, servers := range config.Groups {
			fmt.Printf("    %d: %v\n", gid, servers)
		}
	case "status":
		resp, err := ck.GetStatus()
		if err != nil {
			fmt.Printf("Error getting status: %v\n", err)
			return
		}
		fmt.Printf("Node Status:\n")
		fmt.Printf("  ID: %d\n", resp.Id)
		fmt.Printf("  State: %s\n", resp.State)
		fmt.Printf("  Term: %d\n", resp.Term)
		fmt.Printf("  LastApplied: %d\n", resp.LastApplied)
		fmt.Printf("  CommitIndex: %d\n", resp.CommitIndex)
	default:
		usage()
	}
}

func usage() {
	fmt.Println("Usage: kvclient [options] <command> [args]")
	fmt.Println("Commands:")
	fmt.Println("  join <gid>=<server1>,<server2>...  Add replica groups")
	fmt.Println("  leave <gid1> <gid2>...             Remove replica groups")
	fmt.Println("  move <shard> <gid>                 Move shard to group")
	fmt.Println("  query [num]                        Fetch configuration")
	fmt.Println("  status                             Get cluster status")
}
