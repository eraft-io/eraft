package main

import (
	"flag"
	"fmt"
	"math/rand"
	"strconv"
	"strings"
	"sync"
	"sync/atomic"
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

		// 打印节点信息
		fmt.Printf("%-10s %-25s %-15s %-10s %-10s %-15s %-15s\n", "GID-ID", "Address", "Role", "Term", "Applied", "Commit", "Storage(B)")
		fmt.Println(strings.Repeat("-", 105))
		for _, resp := range results {
			nodeInfo := fmt.Sprintf("%d-%d", resp.Gid, resp.Id)
			fmt.Printf("%-10s %-25s %-15s %-10d %-10d %-15d %-15d\n",
				nodeInfo, resp.Address, resp.State, resp.Term, resp.LastApplied, resp.CommitIndex, resp.StorageSize)

			// 打印该节点的shard统计信息
			if len(resp.Shards) > 0 {
				fmt.Printf("  Shards:\n")
				fmt.Printf("  %-10s %-15s %-15s %-15s\n", "ShardID", "Status", "Keys", "Bytes")
				for _, shard := range resp.Shards {
					fmt.Printf("  %-10d %-15s %-15d %-15d\n", shard.ShardId, shard.Status, shard.Keys, shard.Bytes)
				}
				fmt.Println()
			}
		}
	case "bench":
		if len(flag.Args()) < 2 {
			fmt.Println("Usage: bench <num_requests> [num_clients]")
			fmt.Println("  num_requests: Total number of requests to execute")
			fmt.Println("  num_clients:  Number of concurrent clients (default: 1)")
			return
		}
		num, err := strconv.Atoi(flag.Arg(1))
		if err != nil {
			fmt.Printf("Invalid number of requests: %v\n", err)
			return
		}

		// Parse number of concurrent clients
		numClients := 1
		if len(flag.Args()) >= 3 {
			numClients, err = strconv.Atoi(flag.Arg(2))
			if err != nil || numClients < 1 {
				fmt.Printf("Invalid number of clients, using default: 1\n")
				numClients = 1
			}
		}

		fmt.Printf("Starting benchmark with %d requests using %d concurrent clients...\n", num, numClients)

		// Calculate requests per client
		requestsPerClient := num / numClients
		remainingRequests := num % numClients

		var wg sync.WaitGroup
		var totalSuccess int64
		var totalFailed int64
		start := time.Now()

		// Launch concurrent clients
		for clientID := 0; clientID < numClients; clientID++ {
			wg.Add(1)
			go func(id int) {
				defer wg.Done()

				// Create a new clerk for each client
				clientCk := shardkv.MakeClerk(ctrlerList)

				// Calculate requests for this client
				clientRequests := requestsPerClient
				if id < remainingRequests {
					clientRequests++ // Distribute remaining requests
				}

				// Seed random with client ID for different sequences
				clientRand := rand.New(rand.NewSource(time.Now().UnixNano() + int64(id)))

				for i := 0; i < clientRequests; i++ {
					// Generate random key with random first letter to distribute across shards
					firstLetter := byte('a' + clientRand.Intn(26))
					key := fmt.Sprintf("%c-key-%d-%d-%d", firstLetter, id, i, clientRand.Int63())
					value := fmt.Sprintf("value-%d-%d-%d", id, i, clientRand.Int63())

					clientCk.Put(key, value)
					atomic.AddInt64(&totalSuccess, 1)

					// Progress report from first client
					if id == 0 && (i+1)%100 == 0 {
						fmt.Printf("Progress: %d requests completed...\n", i+1)
					}
				}
			}(clientID)
		}

		// Wait for all clients to complete
		wg.Wait()
		duration := time.Since(start)

		fmt.Printf("\nBenchmark Results:\n")
		fmt.Printf("==================\n")
		fmt.Printf("Total Requests:    %d\n", num)
		fmt.Printf("Concurrent Clients: %d\n", numClients)
		fmt.Printf("Successful:        %d\n", totalSuccess)
		fmt.Printf("Failed:            %d\n", totalFailed)
		fmt.Printf("Duration:          %v\n", duration)
		fmt.Printf("Throughput:        %.2f req/s\n", float64(totalSuccess)/duration.Seconds())
		fmt.Printf("Avg Latency:       %.2f ms\n", float64(duration.Milliseconds())/float64(totalSuccess))
	default:
		usage()
	}
}

func usage() {
	fmt.Println("Usage: shardkvclient [options] <command> [args]")
	fmt.Println("Commands:")
	fmt.Println("  get <key>                    Get value for key")
	fmt.Println("  put <key> <value>            Put value for key")
	fmt.Println("  append <key> <value>         Append value to key")
	fmt.Println("  status                       Get cluster status")
	fmt.Println("  bench <num> [clients]        Run benchmark with random KV pairs")
	fmt.Println("                               num: total requests")
	fmt.Println("                               clients: concurrent clients (default: 1)")
}
