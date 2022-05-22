package main

import (
	"fmt"
	"os"
	"os/signal"
	"strconv"
	"time"

	"github.com/eraft-io/mit6.824lab2product/common"
	"github.com/eraft-io/mit6.824lab2product/shardkvserver"
)

func main() {
	if len(os.Args) < 2 {
		fmt.Println("bench_cli [configserver addr] [count]")
		return
	}
	sigs := make(chan os.Signal, 1)

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan)

	kvCli := shardkvserver.MakeKvClient(os.Args[1])

	count, err := strconv.Atoi(os.Args[2])
	if err != nil {
		panic(err)
	}

	startTs := time.Now()
	for i := 0; i < count; i++ {
		kvCli.Put(common.RandStringRunes(256), common.RandStringRunes(256))
		time.Sleep(time.Millisecond * 100)
	}

	elapsed := time.Since(startTs).Seconds()
	fmt.Printf("cost %f\n", elapsed)

	go func() {
		sig := <-sigs
		fmt.Println(sig)
		os.Exit(-1)
	}()

}
