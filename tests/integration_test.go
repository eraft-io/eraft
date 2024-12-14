package tests

import (
	"fmt"
	"net"
	"os"
	"os/signal"
	"strconv"
	"strings"
	"syscall"
	"testing"
	"time"

	"github.com/eraft-io/eraft/common"
	"github.com/eraft-io/eraft/logger"
	pb "github.com/eraft-io/eraft/raftpb"
	"github.com/eraft-io/eraft/shardkvserver"
	"github.com/stretchr/testify/assert"

	"github.com/eraft-io/eraft/metaserver"
	"google.golang.org/grpc"
	"google.golang.org/grpc/reflection"
)

func RunMetaServer(peerMaps map[int]string, nodeId int) {
	sigs := make(chan os.Signal, 1)
	signal.Notify(sigs, syscall.SIGINT, syscall.SIGTERM)

	metaSvr := metaserver.MakeMetaServer(peerMaps, nodeId)
	lis, err := net.Listen("tcp", peerMaps[nodeId])
	if err != nil {
		fmt.Printf("failed to listen: %v", err)
		return
	}
	s := grpc.NewServer()

	pb.RegisterRaftServiceServer(s, metaSvr)

	sigChan := make(chan os.Signal, 1)

	signal.Notify(sigChan)

	go func() {
		sig := <-sigs
		fmt.Println(sig)
		metaSvr.Rf.CloseEndsConn()
		metaSvr.StopApply()
		os.Exit(-1)
	}()

	reflection.Register(s)
	err = s.Serve(lis)
	if err != nil {
		fmt.Printf("failed to serve: %v", err)
		return
	}
}

func RunShardKvServer(svrPeerMaps map[int]string, nodeId int, groupId int, metaaddrs string) {
	sigs := make(chan os.Signal, 1)
	signal.Notify(sigs, syscall.SIGINT, syscall.SIGTERM)

	shardSvr := shardkvserver.MakeShardKVServer(svrPeerMaps, int64(nodeId), groupId, metaaddrs)
	lis, err := net.Listen("tcp", svrPeerMaps[nodeId])
	if err != nil {
		fmt.Printf("failed to listen: %v", err)
		return
	}
	fmt.Printf("server listen on: %s \n", svrPeerMaps[nodeId])
	s := grpc.NewServer()
	pb.RegisterRaftServiceServer(s, shardSvr)

	sigChan := make(chan os.Signal, 1)

	signal.Notify(sigChan)

	go func() {
		sig := <-sigs
		fmt.Println(sig)
		shardSvr.GetRf().CloseEndsConn()
		shardSvr.CloseApply()
		os.Exit(-1)
	}()

	reflection.Register(s)
	err = s.Serve(lis)
	if err != nil {
		fmt.Printf("failed to serve: %v", err)
		return
	}
}

func AddServerGroup(metaaddrs string, groupId int64, shardserveraddrs string) {
	cfgCli := metaserver.MakeMetaSvrClient(common.UnUsedTid, strings.Split(metaaddrs, ","))
	addrMap := make(map[int64]string)
	addrMap[groupId] = shardserveraddrs
	cfgCli.Join(addrMap)
}

func MoveSlotToServerGroup(metaaddrs string, startSlot int, endSlot int, groupId int) {
	cfgCli := metaserver.MakeMetaSvrClient(common.UnUsedTid, strings.Split(metaaddrs, ","))
	for i := startSlot; i <= endSlot; i++ {
		cfgCli.Move(i, groupId)
	}
}

func TestBasicClusterRW(t *testing.T) {
	// start metaserver cluster
	go RunMetaServer(map[int]string{0: "127.0.0.1:8088", 1: "127.0.0.1:8089", 2: "127.0.0.1:8090"}, 0)
	go RunMetaServer(map[int]string{0: "127.0.0.1:8088", 1: "127.0.0.1:8089", 2: "127.0.0.1:8090"}, 1)
	go RunMetaServer(map[int]string{0: "127.0.0.1:8088", 1: "127.0.0.1:8089", 2: "127.0.0.1:8090"}, 2)
	time.Sleep(time.Second * 5)
	// start shardserver cluster
	go RunShardKvServer(map[int]string{0: "127.0.0.1:6088", 1: "127.0.0.1:6089", 2: "127.0.0.1:6090"}, 0, 1, "127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090")
	go RunShardKvServer(map[int]string{0: "127.0.0.1:6088", 1: "127.0.0.1:6089", 2: "127.0.0.1:6090"}, 1, 1, "127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090")
	go RunShardKvServer(map[int]string{0: "127.0.0.1:6088", 1: "127.0.0.1:6089", 2: "127.0.0.1:6090"}, 2, 1, "127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090")
	time.Sleep(time.Second * 5)
	go RunShardKvServer(map[int]string{0: "127.0.0.1:7088", 1: "127.0.0.1:7089", 2: "127.0.0.1:7090"}, 0, 2, "127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090")
	go RunShardKvServer(map[int]string{0: "127.0.0.1:7088", 1: "127.0.0.1:7089", 2: "127.0.0.1:7090"}, 1, 2, "127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090")
	go RunShardKvServer(map[int]string{0: "127.0.0.1:7088", 1: "127.0.0.1:7089", 2: "127.0.0.1:7090"}, 2, 2, "127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090")
	time.Sleep(time.Second * 5)
	// init meta server
	AddServerGroup("127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090", 1, "127.0.0.1:6088,127.0.0.1:6089,127.0.0.1:6090")
	AddServerGroup("127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090", 2, "127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090")
	MoveSlotToServerGroup("127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090", 0, 4, 1)
	MoveSlotToServerGroup("127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090", 5, 9, 2)
	time.Sleep(time.Second * 20)

	// R-W test
	shardkvcli := shardkvserver.MakeKvClient("127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090")

	shardkvcli.Put("testKey", "testValue")

	time.Sleep(time.Second * 10)
	val, err := shardkvcli.Get("testKey")
	if err != nil {
		panic(err.Error())
	}
	assert.Equal(t, val, "testValue")
	time.Sleep(time.Second * 3)
	common.RemoveDir("./data")
}

func TestClusterSingleShardRwBench(t *testing.T) {
	// start metaserver cluster
	go RunMetaServer(map[int]string{0: "127.0.0.1:8088", 1: "127.0.0.1:8089", 2: "127.0.0.1:8090"}, 0)
	go RunMetaServer(map[int]string{0: "127.0.0.1:8088", 1: "127.0.0.1:8089", 2: "127.0.0.1:8090"}, 1)
	go RunMetaServer(map[int]string{0: "127.0.0.1:8088", 1: "127.0.0.1:8089", 2: "127.0.0.1:8090"}, 2)
	time.Sleep(time.Second * 5)
	// start shardserver cluster
	go RunShardKvServer(map[int]string{0: "127.0.0.1:6088", 1: "127.0.0.1:6089", 2: "127.0.0.1:6090"}, 0, 1, "127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090")
	go RunShardKvServer(map[int]string{0: "127.0.0.1:6088", 1: "127.0.0.1:6089", 2: "127.0.0.1:6090"}, 1, 1, "127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090")
	go RunShardKvServer(map[int]string{0: "127.0.0.1:6088", 1: "127.0.0.1:6089", 2: "127.0.0.1:6090"}, 2, 1, "127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090")
	time.Sleep(time.Second * 5)
	// init meta server
	AddServerGroup("127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090", 1, "127.0.0.1:6088,127.0.0.1:6089,127.0.0.1:6090")
	MoveSlotToServerGroup("127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090", 0, 9, 1)
	time.Sleep(time.Second * 20)

	// R-W test
	shardkvcli := shardkvserver.MakeKvClient("127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090")

	N := 64
	KeySize := 64
	ValSize := 64
	benchKvs := map[string]string{}
	for i := 0; i < N; i++ {
		k := strconv.Itoa(i) + "-" + common.RandStringRunes(KeySize)
		v := common.RandStringRunes(ValSize)
		benchKvs[k] = v
	}
	costTime := []int64{}

	for key, val := range benchKvs {
		start := time.Now()
		shardkvcli.Put(key, val)
		elapsed := time.Since(start)
		costTime = append(costTime, elapsed.Milliseconds())
	}

	sum := 0.0
	avg := 0.0
	maxi := 0.0
	mini := 9999999999999999.0

	for _, cost := range costTime {
		sum += float64(cost)
		if cost > int64(maxi) {
			maxi = float64(cost)
		}
		if cost < int64(mini) {
			mini = float64(cost)
		}
	}
	avg = sum / float64(len(costTime))
	logger.ELogger().Sugar().Debugf("total request: %d", N)
	logger.ELogger().Sugar().Debugf("total time cost: %f", sum)
	logger.ELogger().Sugar().Debugf("avg time cost: %f", avg)
	logger.ELogger().Sugar().Debugf("max time cost: %f", maxi)
	logger.ELogger().Sugar().Debugf("min time cost: %f", mini)
	time.Sleep(time.Second * 2)
	common.RemoveDir("./data")
}

func TestClusterRwBench(t *testing.T) {
	// start metaserver cluster
	go RunMetaServer(map[int]string{0: "127.0.0.1:8088", 1: "127.0.0.1:8089", 2: "127.0.0.1:8090"}, 0)
	go RunMetaServer(map[int]string{0: "127.0.0.1:8088", 1: "127.0.0.1:8089", 2: "127.0.0.1:8090"}, 1)
	go RunMetaServer(map[int]string{0: "127.0.0.1:8088", 1: "127.0.0.1:8089", 2: "127.0.0.1:8090"}, 2)
	time.Sleep(time.Second * 5)
	// start shardserver cluster
	go RunShardKvServer(map[int]string{0: "127.0.0.1:6088", 1: "127.0.0.1:6089", 2: "127.0.0.1:6090"}, 0, 1, "127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090")
	go RunShardKvServer(map[int]string{0: "127.0.0.1:6088", 1: "127.0.0.1:6089", 2: "127.0.0.1:6090"}, 1, 1, "127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090")
	go RunShardKvServer(map[int]string{0: "127.0.0.1:6088", 1: "127.0.0.1:6089", 2: "127.0.0.1:6090"}, 2, 1, "127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090")
	time.Sleep(time.Second * 5)
	go RunShardKvServer(map[int]string{0: "127.0.0.1:7088", 1: "127.0.0.1:7089", 2: "127.0.0.1:7090"}, 0, 2, "127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090")
	go RunShardKvServer(map[int]string{0: "127.0.0.1:7088", 1: "127.0.0.1:7089", 2: "127.0.0.1:7090"}, 1, 2, "127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090")
	go RunShardKvServer(map[int]string{0: "127.0.0.1:7088", 1: "127.0.0.1:7089", 2: "127.0.0.1:7090"}, 2, 2, "127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090")
	time.Sleep(time.Second * 5)
	// init meta server
	AddServerGroup("127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090", 1, "127.0.0.1:6088,127.0.0.1:6089,127.0.0.1:6090")
	AddServerGroup("127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090", 2, "127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090")
	MoveSlotToServerGroup("127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090", 0, 4, 1)
	MoveSlotToServerGroup("127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090", 5, 9, 2)
	time.Sleep(time.Second * 20)

	// R-W test
	shardKVCli := shardkvserver.MakeKvClient("127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090")

	N := 64
	KeySize := 64
	ValSize := 64
	benchKvs := map[string]string{}
	for i := 0; i < N; i++ {
		k := strconv.Itoa(i) + "-" + common.RandStringRunes(KeySize)
		v := common.RandStringRunes(ValSize)
		benchKvs[k] = v
	}
	costTime := []int64{}

	for key, val := range benchKvs {
		start := time.Now()
		shardKVCli.Put(key, val)
		elapsed := time.Since(start)
		costTime = append(costTime, elapsed.Milliseconds())
	}

	sum := 0.0
	avg := 0.0
	maxi := 0.0
	mini := 9999999999999999.0

	for _, cost := range costTime {
		sum += float64(cost)
		if cost > int64(maxi) {
			maxi = float64(cost)
		}
		if cost < int64(mini) {
			mini = float64(cost)
		}
	}
	avg = sum / float64(len(costTime))
	logger.ELogger().Sugar().Debugf("total request: %d", N)
	logger.ELogger().Sugar().Debugf("total time cost: %f", sum)
	logger.ELogger().Sugar().Debugf("avg time cost: %f", avg)
	logger.ELogger().Sugar().Debugf("max time cost: %f", maxi)
	logger.ELogger().Sugar().Debugf("min time cost: %f", mini)

	time.Sleep(time.Second * 5)
	common.RemoveDir("./data")
}
