package main

import (
	"fmt"
	"os"
	"os/signal"
	"strconv"
	"strings"
	"time"

	"github.com/eraft-io/eraft/common"
	"github.com/eraft-io/eraft/configserver"
	"github.com/eraft-io/eraft/shardkvserver"
)

func main() {
	if len(os.Args) < 2 {
		fmt.Println("bench_cli [configserver addr] [count] [type]")
		return
	}
	sigs := make(chan os.Signal, 1)

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan)

	kvCli := shardkvserver.MakeKvClient(os.Args[1])
	addrs := strings.Split(os.Args[1], ",")
	cfgCli := configserver.MakeCfgSvrClient(common.UN_UNSED_TID, addrs)

	count, err := strconv.Atoi(os.Args[2])
	if err != nil {
		panic(err)
	}

	startTs := time.Now()

	switch os.Args[3] {
	case "put":
		for i := 0; i < count; i++ {
			kvCli.Put(common.RandStringRunes(64), common.RandStringRunes(64))
			// time.Sleep(time.Millisecond * 5)
		}
	case "query":
		for i := 0; i < count; i++ {
			cfgCli.Query(-1)
			time.Sleep(1 * time.Millisecond)
		}
	}

	elapsed := time.Since(startTs).Seconds()
	fmt.Printf("total cost %f s\n", elapsed)

	kvCli.CloseRpcCliConn()

	go func() {
		sig := <-sigs
		fmt.Println(sig)
		os.Exit(-1)
	}()

}
