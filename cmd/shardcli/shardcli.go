//
// MIT License

// Copyright (c) 2022 eraft dev group

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

package main

import (
	"encoding/json"
	"fmt"
	"os"
	"os/signal"
	"strconv"
	"strings"

	"github.com/eraft-io/eraft/raftcore"
	"github.com/eraft-io/eraft/shardkvserver"
)

func main() {
	if len(os.Args) < 3 {
		fmt.Printf("usage: \n" +
			"kvcli [configserver addr] put [key] [value]\n" +
			"kvcli [configserver addr] get [key]\n" +
			"kvcli [configserver addr] getbuckets [gid] [id1,id2,...]\n" +
			"kvcli [configserver addr] delbuckets [gid] [id1,id2,...]\n" +
			"kvcli [configserver addr] insertbucketkv [gid] [bid] [key] [value]\n" +
			"kvcli [configserver addr] migrate [start-end] [from gid] [to gid]\n")
		return
	}
	sigs := make(chan os.Signal, 1)
	shardKvCli := shardkvserver.MakeKvClient(os.Args[1])

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan)

	switch os.Args[2] {
	case "put":
		if err := shardKvCli.Put(os.Args[3], os.Args[4]); err != nil {
			fmt.Println("err: " + err.Error())
			return
		}
	case "get":
		v, err := shardKvCli.Get(os.Args[3])
		if err != nil {
			fmt.Println("err: " + err.Error())
			return
		}
		fmt.Println("got value: " + v)
	case "getbuckets":
		gid, _ := strconv.Atoi(os.Args[3])
		bidsStr := os.Args[4]
		bids := []int64{}
		bidsStrArr := strings.Split(bidsStr, ",")
		for _, bidStr := range bidsStrArr {
			bid, _ := strconv.Atoi(bidStr)
			bids = append(bids, int64(bid))
		}
		datas := shardKvCli.GetBucketDatas(gid, bids)
		fmt.Println("get buckets datas: " + string(datas))
	case "delbuckets":
		gid, _ := strconv.Atoi(os.Args[3])
		bidsStr := os.Args[4]
		bids := []int64{}
		bidsStrArr := strings.Split(bidsStr, ",")
		for _, bidStr := range bidsStrArr {
			bid, _ := strconv.Atoi(bidStr)
			bids = append(bids, int64(bid))
		}
		shardKvCli.DeleteBucketDatas(gid, bids)
	case "insertbucketkv":
		gid, _ := strconv.Atoi(os.Args[3])
		bid, _ := strconv.Atoi(os.Args[4])
		bucketDatas := &shardkvserver.BucketDatasVo{}
		bucketDatas.Datas = make(map[int]map[string]string)
		kv := map[string]string{os.Args[5]: os.Args[6]}
		bucketDatas.Datas[bid] = kv
		datas, _ := json.Marshal(bucketDatas)
		shardKvCli.InsertBucketDatas(gid, []int64{int64(bid)}, datas)
	case "migrate":
		items := strings.Split(os.Args[3], "-")
		if len(items) != 2 {
			raftcore.PrintDebugLog("bucket range args error")
			return
		}
		start, _ := strconv.Atoi(items[0])
		end, _ := strconv.Atoi(items[1])
		fromGid, _ := strconv.Atoi(os.Args[4])
		toGid, _ := strconv.Atoi(os.Args[5])
		// get datas from source group
		bids := []int64{}
		for i := start; i <= end; i++ {
			bids = append(bids, int64(i))
		}
		datas := shardKvCli.GetBucketDatas(fromGid, bids)
		bucketDatas := &shardkvserver.BucketDatasVo{}
		bucketDatas.Datas = make(map[int]map[string]string)
		json.Unmarshal(datas, bucketDatas)
		bucketDatasNew := &shardkvserver.BucketDatasVo{}
		bucketDatasNew.Datas = make(map[int]map[string]string)
		for bid, kvs := range bucketDatas.Datas {
			newkv := map[string]string{}
			for k, v := range kvs {
				newkv[strings.Split(k, "$^$")[1]] = v
			}
			bucketDatasNew.Datas[bid] = newkv
		}
		fmt.Printf("get datas %v from source group\n", bucketDatasNew)
		newDatas, _ := json.Marshal(bucketDatasNew)
		// insert data to dest group
		shardKvCli.InsertBucketDatas(toGid, bids, newDatas)
		fmt.Printf("insert datas %v to dest group %d\n", bids, toGid)
		// update metaserver config
		for _, bid := range bids {
			shardKvCli.GetCsClient().Move(int(bid), toGid)
		}
		fmt.Printf("update metaserver gid %d bids %v config", toGid, bids)
		// delete datas from source group
		shardKvCli.DeleteBucketDatas(fromGid, bids)
		fmt.Printf("delete datas %v from source group %d\n", bids, fromGid)
	}

	go func() {
		sig := <-sigs
		fmt.Println(sig)
		for _, cli := range shardKvCli.GetCsClient().GetRpcClis() {
			cli.CloseAllConn()
		}
		shardKvCli.GetRpcClient().CloseAllConn()
		os.Exit(-1)
	}()
}
