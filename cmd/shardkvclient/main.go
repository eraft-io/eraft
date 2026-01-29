package main

import (
	"flag"
	"fmt"
	"strings"

	"github.com/eraft-io/eraft/shardkv"
)

func main() {
	ctrlers := flag.String("ctrlers", "localhost:50051,localhost:50052,localhost:50053", "comma separated addresses of shardctrler nodes")
	flag.Parse()

	ctrlerList := strings.Split(*ctrlers, ",")
	ck := shardkv.MakeClerk(ctrlerList)

	if len(flag.Args()) < 1 {
		usage()
		return
	}

	cmd := flag.Arg(0)
	switch cmd {
	case "get":
		if len(flag.Args()) < 2 {
			fmt.Println("Usage: get <key>")
			return
		}
		key := flag.Arg(1)
		val := ck.Get(key)
		fmt.Printf("%s\n", val)
	case "put":
		if len(flag.Args()) < 3 {
			fmt.Println("Usage: put <key> <value>")
			return
		}
		key := flag.Arg(1)
		val := flag.Arg(2)
		ck.Put(key, val)
		fmt.Println("Put OK")
	case "append":
		if len(flag.Args()) < 3 {
			fmt.Println("Usage: append <key> <value>")
			return
		}
		key := flag.Arg(1)
		val := flag.Arg(2)
		ck.Append(key, val)
		fmt.Println("Append OK")
	case "status":
		results := ck.GetStatus()
		fmt.Printf("ShardKV Cluster Status:\n")
		for _, resp := range results {
			fmt.Printf("  Node %d (GID %d): %s, Term: %d, Applied: %d, Commit: %d\n",
				resp.Id%100, resp.Id/100, resp.State, resp.Term, resp.LastApplied, resp.CommitIndex)
		}
	default:
		usage()
	}
}

func usage() {
	fmt.Println("Usage: shardkvclient [options] <command> [args]")
	fmt.Println("Commands:")
	fmt.Println("  get <key>            Get value for key")
	fmt.Println("  put <key> <value>    Put value for key")
	fmt.Println("  append <key> <value> Append value to key")
	fmt.Println("  status               Get cluster status")
}
