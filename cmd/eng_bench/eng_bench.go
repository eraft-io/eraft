package main

import (
	"fmt"
	"os"
	"strconv"
	"sync"
	"time"

	"github.com/eraft-io/eraft/common"
	"github.com/eraft-io/eraft/storage_eng"
)

func main() {

	if len(os.Args) < 2 {
		fmt.Println("bench_pmem [key_size] [value_size] [leveldb|pmem] [count] [thread count]")
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
	count, err := strconv.Atoi(os.Args[4])
	if err != nil {
		panic(err)
	}
	threadCount, err := strconv.Atoi(os.Args[5])
	if err != nil {
		panic(err)
	}

	keys := make([]string, count)
	vals := make([]string, count)

	startTs := time.Now()

	for i := 0; i < count; i++ {
		rndK := common.RandStringRunes(keySize)
		rndV := common.RandStringRunes(valSize)
		keys[i] = rndK
		vals[i] = rndV
	}

	switch os.Args[3] {
	case "pmem":
		fmt.Printf("start pmem bench\n")

		startTs = time.Now()

		wg := sync.WaitGroup{}
		for idx := 0; idx < threadCount; idx++ {
			pmemDBEng, err := storage_eng.MakePMemKvStore("127.0.0.1:6379")
			if err != nil {
				panic(err)
			}
			wg.Add(1)
			go func() {
				defer wg.Done()
				for i := 0; i < count; i++ {
					pmemDBEng.Put(keys[i], vals[i])
				}
			}()
		}
		wg.Wait()

		elapsed := time.Since(startTs).Seconds()
		fmt.Printf("total cost %f s\n", elapsed)
		// for i := 0; i < count; i++ {
		// 	val, err := pmemDBEng.Get(keys[i])
		// 	if err != nil {
		// 		panic(err)
		// 	}
		// 	if val != vals[i] {
		// 		fmt.Printf("check value %d no ok!\n", i)
		// 	}
		// }
	case "leveldb":
		levelDBEng, err := storage_eng.MakeLevelDBKvStore("./leveldb_data")
		if err != nil {
			panic(err)
		}
		startTs = time.Now()
		wg := sync.WaitGroup{}

		for idx := 0; idx < threadCount; idx++ {
			wg.Add(1)
			go func() {
				defer wg.Done()
				for i := 0; i < count; i++ {
					levelDBEng.Put(keys[i], vals[i])
				}
			}()
		}
		wg.Wait()

		elapsed := time.Since(startTs).Seconds()
		fmt.Printf("total cost %f s\n", elapsed)
		// for i := 0; i < count; i++ {
		// 	val, err := levelDBEng.Get(keys[i])
		// 	if err != nil {
		// 		panic(err)
		// 	}
		// 	if val != vals[i] {
		// 		fmt.Printf("check value %d no ok!\n", i)
		// 	}
		// }
	}
}
