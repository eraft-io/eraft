package main

import (
	"context"
	"crypto/rand"
	"fmt"
	"math/big"
	"os"
	"os/signal"
	"strconv"
	"time"

	pb "github.com/eraft-io/eraft/raftpb"

	"github.com/eraft-io/eraft/common"
	"github.com/eraft-io/eraft/raftcore"
	"github.com/eraft-io/eraft/storage_eng"
)

// !!! need go-pmem go compiler

var ctx = context.Background()

type KvClient struct {
	rpcCli    *raftcore.RaftClientEnd
	leaderId  int64
	clientId  int64
	commandId int64
}

func (kvCli *KvClient) Close() {
	kvCli.rpcCli.CloseAllConn()
}

func nrand() int64 {
	max := big.NewInt(int64(1) << 62)
	bigx, _ := rand.Int(rand.Reader, max)
	return bigx.Int64()
}

func MakeKvClient(targetId int, targetAddr string) *KvClient {
	cli := raftcore.MakeRaftClientEnd(targetAddr, uint64(targetId))
	return &KvClient{
		rpcCli:    cli,
		leaderId:  0,
		clientId:  nrand(),
		commandId: 0,
	}
}

func (kvCli *KvClient) Put(key, value string) string {
	cmdReq := &pb.CommandRequest{
		Key:      key,
		Value:    value,
		ClientId: kvCli.clientId,
		OpType:   pb.OpType_OpPut,
	}
	_, err := (*kvCli.rpcCli.GetRaftServiceCli()).DoCommand(context.Background(), cmdReq)
	if err != nil {
		return "err"
	}
	return "ok"
}

func (kvCli *KvClient) Get(key string) (error, string) {
	cmdReq := &pb.CommandRequest{
		Key:      key,
		OpType:   pb.OpType_OpGet,
		ClientId: kvCli.clientId,
	}
	resp, err := (*kvCli.rpcCli.GetRaftServiceCli()).DoCommand(context.Background(), cmdReq)
	if err != nil {
		return err, ""
	}
	return nil, resp.Value
}

func main() {

	if len(os.Args) < 2 {
		fmt.Println("bench_pmem [key_size] [value_size] [leveldb|pmem|pmem_ex] [count]")
		return
	}

	sigs := make(chan os.Signal, 1)
	kvCli := MakeKvClient(99, "127.0.0.1:8088")
	sigChan := make(chan os.Signal)
	signal.Notify(sigChan)

	go func() {
		sig := <-sigs
		fmt.Println(sig)
		kvCli.Close()
		os.Exit(-1)
	}()

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
	case "pmem_dict":
		fmt.Printf("start pmem_dict bench\n")
		for i := 0; i < count; i++ {
			kvCli.Put(keys[i], vals[i])
		}
		elapsed := time.Since(startTs).Seconds()
		fmt.Printf("total cost %f s\n", elapsed)
		for i := 0; i < count; i++ {
			err, val := kvCli.Get(keys[i])
			if err != nil {
				panic(err)
			}
			if val != vals[i] {
				fmt.Printf("check value %d no ok!\n", i)
			}
		}
	case "leveldb":
		levelDBEng, err := storage_eng.MakeLevelDBKvStore("./leveldb_data")
		if err != nil {
			panic(err)
		}
		startTs = time.Now()
		for i := 0; i < count; i++ {
			levelDBEng.Put(keys[i], vals[i])
		}
		elapsed := time.Since(startTs).Seconds()
		fmt.Printf("total cost %f s\n", elapsed)
		for i := 0; i < count; i++ {
			val, err := levelDBEng.Get(keys[i])
			if err != nil {
				panic(err)
			}
			if val != vals[i] {
				fmt.Printf("check value %d no ok!\n", i)
			}
		}
	}
}
