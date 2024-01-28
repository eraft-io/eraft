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

	"github.com/eraft-io/eraft/shardkvserver"
)

func main() {
	if len(os.Args) < 3 {
		fmt.Printf("usage: \n" +
			"kvcli [configserver addr] put [key] [value]\n" +
			"kvcli [configserver addr] get [key]\n" +
			"kvcli [configserver addr] getbuckets [gid] [id1,id2,...]\n" +
			"kvcli [configserver addr] delbuckets [gid] [id1,id2,...]\n" +
			"kvcli [configserver addr] insertbucketkv [gid] [bid] [key] [value]\n")
		return
	}
	sigs := make(chan os.Signal, 1)

	shard_kvcli := shardkvserver.MakeKvClient(os.Args[1])

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan)

	switch os.Args[2] {
	case "put":
		if err := shard_kvcli.Put(os.Args[3], os.Args[4]); err != nil {
			fmt.Println("err: " + err.Error())
			return
		}
	case "get":
		v, err := shard_kvcli.Get(os.Args[3])
		if err != nil {
			fmt.Println("err: " + err.Error())
			return
		}
		fmt.Println("got value: " + v)
	case "getbuckets":
		gid, _ := strconv.Atoi(os.Args[3])
		bidsStr := os.Args[4]
		bids := []int64{}
		bids_strarr := strings.Split(bidsStr, ",")
		for _, bidStr := range bids_strarr {
			bid, _ := strconv.Atoi(bidStr)
			bids = append(bids, int64(bid))
		}
		datas := shard_kvcli.GetBucketDatas(gid, bids)
		fmt.Println("get buckets datas: " + datas)
	case "delbuckets":
		gid, _ := strconv.Atoi(os.Args[3])
		bidsStr := os.Args[4]
		bids := []int64{}
		bids_strarr := strings.Split(bidsStr, ",")
		for _, bidStr := range bids_strarr {
			bid, _ := strconv.Atoi(bidStr)
			bids = append(bids, int64(bid))
		}
		shard_kvcli.DeleteBucketDatas(gid, bids)
	case "insertbucketkv":
		gid, _ := strconv.Atoi(os.Args[3])
		bid, _ := strconv.Atoi(os.Args[4])
		bucket_datas := &shardkvserver.BucketDatasVo{}
		bucket_datas.Datas = make(map[int]map[string]string)
		kv := map[string]string{os.Args[5]: os.Args[6]}
		bucket_datas.Datas[bid] = kv
		datas, _ := json.Marshal(bucket_datas)
		shard_kvcli.InsertBucketDatas(gid, []int64{int64(bid)}, datas)
	}
	go func() {
		sig := <-sigs
		fmt.Println(sig)
		for _, cli := range shard_kvcli.GetCsClient().GetRpcClis() {
			cli.CloseAllConn()
		}
		shard_kvcli.GetRpcClient().CloseAllConn()
		os.Exit(-1)
	}()
}
