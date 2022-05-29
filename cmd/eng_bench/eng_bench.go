package main

import (
	"fmt"
	"os"
	"strconv"
	"time"

	"github.com/eraft-io/eraft/common"
	pmem_redis "github.com/eraft-io/eraft/go-redis-pmem/redis"
	"github.com/eraft-io/eraft/storage_eng"
)

// !!! need go-pmem go compiler

func main() {

	if len(os.Args) < 2 {
		fmt.Println("bench_pmem [key_size] [value_size] [leveldb|pmem]")
		return
	}

	keySize, err := strconv.Atoi(os.Args[1])
	if err != nil {
		panic(err)
	}
	valSize, err := strconv.Atoi(os.Args[2])
	if err != nil {
		panic(err)
	}

	const KCount int = 1000

	startTs := time.Now()

	var keys, vals [KCount]string

	for i := 0; i < KCount; i++ {
		rndK := common.RandStringRunes(keySize)
		rndV := common.RandStringRunes(valSize)
		keys[i] = rndK
		vals[i] = rndV
	}

	switch os.Args[3] {
	case "pmem":
		pmemEng, err := pmem_redis.MakePMemKvStore("./list_database")
		if err != nil {
			panic(err)
		}
		for i := 0; i < KCount; i++ {
			pmemEng.Put(keys[i], vals[i])
		}

		elapsed := time.Since(startTs).Seconds()
		fmt.Printf("total cost %f s\n", elapsed)
		for i := 0; i < KCount; i++ {
			val, err := pmemEng.Get(keys[i])
			if err != nil {
				panic(err)
			}
			if val != vals[i] {
				fmt.Printf("check value %d no ok!\n", i)
			} else {
				// fmt.Printf("check value %d ok!\n", i)
			}
		}
	case "leveldb":
		levelDBEng, err := storage_eng.MakeLevelDBKvStore("./leveldb_data")
		if err != nil {
			panic(err)
		}
		for i := 0; i < KCount; i++ {
			levelDBEng.Put(keys[i], vals[i])
		}
		elapsed := time.Since(startTs).Seconds()
		fmt.Printf("total cost %f s\n", elapsed)
		for i := 0; i < KCount; i++ {
			val, err := levelDBEng.Get(keys[i])
			if err != nil {
				panic(err)
			}
			if val != vals[i] {
				fmt.Printf("check value %d no ok!\n", i)
			} else {
				// fmt.Printf("check value %d ok!\n", i)
			}
		}
	}
}
