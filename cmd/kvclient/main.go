package main

import (
	"flag"
	"fmt"
	"strings"

	"github.com/eraft-io/eraft/kvraft"
)

func main() {
	addrs := flag.String("addrs", "localhost:5001,localhost:5002,localhost:5003", "comma separated addresses")
	op := flag.String("op", "get", "operation: get, put, append, status")
	key := flag.String("key", "", "key")
	value := flag.String("value", "", "value")
	flag.Parse()

	addrList := strings.Split(*addrs, ",")
	ck := kvraft.MakeClerk(addrList)

	switch *op {
	case "get":
		val := ck.Get(*key)
		fmt.Printf("Get(%s) -> %s\n", *key, val)
	case "put":
		ck.Put(*key, *value)
		fmt.Printf("Put(%s, %s) success\n", *key, *value)
	case "append":
		ck.Append(*key, *value)
		fmt.Printf("Append(%s, %s) success\n", *key, *value)
	case "status":
		stats := ck.GetStatus()
		fmt.Printf("%-5s | %-10s | %-5s | %-12s | %-12s\n", "ID", "State", "Term", "LastApplied", "CommitIndex")
		fmt.Println(strings.Repeat("-", 60))
		for _, s := range stats {
			if s.State == "Offline" {
				fmt.Printf("%-5d | %-10s | %-5s | %-12s | %-12s\n", s.Id, s.State, "-", "-", "-")
			} else {
				fmt.Printf("%-5d | %-10s | %-5d | %-12d | %-12d\n", s.Id, s.State, s.Term, s.LastApplied, s.CommitIndex)
			}
		}
	default:
		fmt.Println("Unknown operation")
	}
}
