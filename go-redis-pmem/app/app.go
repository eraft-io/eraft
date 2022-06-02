package main

import pmem_redis "github.com/eraft-io/eraft/go-redis-pmem/redis"

func main() {
	pmem_redis.RunServer()
}
