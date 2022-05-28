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

	"github.com/eraft-io/eraft/common"
	"github.com/eraft-io/eraft/configserver"
	"github.com/eraft-io/eraft/raftcore"
)

func main() {
	if len(os.Args) < 3 {
		fmt.Printf("usage: \n " +
			"join a group:  kvcli [config server addrs] join [gid] [svraddr1,svraddr2,svraddr3]\n " +
			"leave a group: kvcli [config server addrs] leave [gid]\n " +
			"query newlest config: kvcli [config server addrs] query\n " +
			"move a bucket: kvcli [config server addrs] move [start-end] [gid]\n")
		return
	}
	sigs := make(chan os.Signal, 1)

	addrs := strings.Split(os.Args[1], ",")
	cfgCli := configserver.MakeCfgSvrClient(common.UN_UNSED_TID, addrs)

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan)

	go func() {
		sig := <-sigs
		fmt.Println(sig)
		for _, cli := range cfgCli.GetRpcClis() {
			cli.CloseAllConn()
		}
		os.Exit(-1)
	}()

	switch os.Args[2] {
	case "join":
		{
			gid, _ := strconv.Atoi(os.Args[3])
			addrMap := make(map[int64]string)
			addrMap[int64(gid)] = os.Args[4]
			cfgCli.Join(addrMap)
		}
	case "leave":
		{
			gid, _ := strconv.Atoi(os.Args[3])
			cfgCli.Leave([]int64{int64(gid)})
		}
	case "query":
		{
			lastConf := cfgCli.Query(-1)
			outBytes, _ := json.Marshal(lastConf)
			raftcore.PrintDebugLog("latest configuration: " + string(outBytes))
		}
	case "move":
		{ //3 [a-b] 4 gid
			items := strings.Split(os.Args[3], "-")
			if len(items) != 2 {
				raftcore.PrintDebugLog("bucket range args error")
				return
			}
			start, _ := strconv.Atoi(items[0])
			end, _ := strconv.Atoi(items[1])
			gid, _ := strconv.Atoi(os.Args[4])

			for i := start; i <= end; i++ {
				cfgCli.Move(i, gid)
			}
		}
	}
}
