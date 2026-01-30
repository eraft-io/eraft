package main

import (
	"flag"
	"fmt"
	"math/rand"
	"strconv"
	"strings"
	"time"

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
		results, err := ck.GetStatus()
		if err != nil {
			fmt.Printf("Error getting status: %v\n", err)
			return
		}
		fmt.Printf("%-10s %-25s %-15s %-10s %-10s %-15s %-15s\n", "GID-ID", "Address", "Role", "Term", "Applied", "Commit", "Storage(B)")
		fmt.Println(strings.Repeat("-", 105))
		for _, resp := range results {
			nodeInfo := fmt.Sprintf("%d-%d", resp.Gid, resp.Id)
			fmt.Printf("%-10s %-25s %-15s %-10d %-10d %-15d %-15d\n",
				nodeInfo, resp.Address, resp.State, resp.Term, resp.LastApplied, resp.CommitIndex, resp.StorageSize)
		}
	case "bench":
		if len(flag.Args()) < 2 {
			fmt.Println("Usage: bench <num_requests>")
			return
		}
		num, err := strconv.Atoi(flag.Arg(1))
		if err != nil {
			fmt.Printf("Invalid number of requests: %v\n", err)
			return
		}
		fmt.Printf("Starting benchmark with %d requests...\n", num)
		start := time.Now()
		for i := 0; i < num; i++ {
			// Generate random key with random first letter to distribute across shards
			firstLetter := byte('a' + rand.Intn(26))
			key := fmt.Sprintf("%c-key-%d-%d", firstLetter, i, rand.Int63())
			value := fmt.Sprintf("value-%d-%d", i, rand.Int63())
			ck.Put(key, value)
			if (i+1)%100 == 0 {
				fmt.Printf("Finished %d requests...\n", i+1)
			}
		}
		duration := time.Since(start)
		fmt.Printf("Benchmark finished: %d requests in %v (%.2f req/s)\n",
			num, duration, float64(num)/duration.Seconds())
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
	fmt.Println("  bench <num>          Run benchmark with random KV pairs")
}
