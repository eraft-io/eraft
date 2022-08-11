// Copyright [2022] [WellWood] [wellwood-x@googlegroups.com]

// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

// 	http://www.apache.org/licenses/LICENSE-2.0

// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package main

import (
	"bufio"
	"flag"
	"fmt"
	"io"
	"net/http"
	"os"
	"strings"

	"github.com/eraft-io/eraft/pkg/blockserver"
	"github.com/eraft-io/eraft/pkg/clientsdk"
	"github.com/eraft-io/eraft/pkg/common"
	"github.com/eraft-io/eraft/pkg/consts"
	"github.com/eraft-io/eraft/pkg/log"
	pb "github.com/eraft-io/eraft/pkg/protocol"
)

const tpl = `<html>
<head>
<title>上传文件</title>
</head>
<body>
<form enctype="multipart/form-data" action="/upload" method="post">
 <input type="file" name="uploadfile" />
 <input type="hidden" name="token" value="{...{.}...}"/>
 <input type="submit" value="upload" />
</form>
</body>
</html>`

var metaNodeAddrs = flag.String("meta_addrs", "wellwood-metaserver-0.wellwood-metaserver:8088,wellwood-metaserver-1.wellwood-metaserver:8089,wellwood-metaserver-2.wellwood-metaserver:8090", "input block server node addrs")

func index(w http.ResponseWriter, r *http.Request) {
	w.Write([]byte(tpl))
}

func upload(w http.ResponseWriter, r *http.Request) {
	r.ParseMultipartForm(32 << 20)
	file, handler, err := r.FormFile("uploadfile")
	if err != nil {
		fmt.Println(err)
		return
	}
	defer file.Close()
	f, err := os.OpenFile(handler.Filename, os.O_WRONLY|os.O_CREATE, 0666)
	if err != nil {
		fmt.Println(err)
		return
	}
	defer f.Close()
	// upload to wellwood
	c := clientsdk.NewClient(*metaNodeAddrs, "", "")
	fileReader := bufio.NewReader(f)
	blockBuf := make([]byte, consts.FILE_BLOCK_SIZE)
	fileBlockMetas := []*pb.FileBlockMeta{}
	objRandId := common.GenGoogleUUID()
	index := 0
	for {
		n, err := fileReader.Read(blockBuf)
		if err != nil && err != io.EOF {
			w.Write([]byte(err.Error()))
		}
		// read last file block
		if n == 0 {
			break
		}
		// query server group meta
		req := pb.ServerGroupMetaConfigRequest{
			ConfigVersion: -1,
			OpType:        pb.ConfigServerGroupMetaOpType_OP_SERVER_GROUP_QUERY,
		}
		serverGroupMetaResp := c.GetMetaSvrCli().CallServerGroupMeta(&req)
		if err != nil {
			w.Write([]byte(err.Error()))
		}
		blockStr := ""
		if n < 64 {
			blockStr = string(blockBuf[:n])
		} else {
			blockStr = string(blockBuf[:64])
		}
		slot := common.StrToSlot(blockStr)
		blockMeta := &pb.FileBlockMeta{
			BlockId:     int64(index),
			BlockSlotId: int64(slot),
		}
		slotsToGroupArr := serverGroupMetaResp.ServerGroupMetas.Slots
		serverGroupAddrs := serverGroupMetaResp.ServerGroupMetas.ServerGroups[slotsToGroupArr[slot]]
		serverAddrArr := strings.Split(serverGroupAddrs, ",")
		blockSvrCli := blockserver.MakeBlockServerClient(serverAddrArr)
		fileBlockRequest := &pb.FileBlockOpRequest{
			FileName:       objRandId,
			FileBlocksMeta: blockMeta,
			BlockContent:   blockBuf[:n],
			OpType:         pb.FileBlockOpType_OP_BLOCK_WRITE,
		}
		writeBlockResp := blockSvrCli.CallFileBlockOp(fileBlockRequest)
		if err != nil {
			w.Write([]byte(err.Error()))
		}
		if writeBlockResp.ErrCode != pb.ErrCode_NO_ERR {
			w.Write([]byte(err.Error()))
		}
		fileBlockMetas = append(fileBlockMetas, blockMeta)
		index += 1
	}
	// wirte obj meta to meta server
	req := pb.ServerGroupMetaConfigRequest{
		OpType: pb.ConfigServerGroupMetaOpType_OP_OSS_OBJECT_PUT,
		BucketOpReq: &pb.BucketOpRequest{
			Object: &pb.Object{
				ObjectId:         handler.Filename,
				ObjectName:       objRandId,
				FromBucketId:     consts.DEFAULT_BUCKET_ID,
				ObjectBlocksMeta: fileBlockMetas,
			},
			BucketId: consts.DEFAULT_BUCKET_ID,
		},
	}
	serverGroupMetaResp := c.GetMetaSvrCli().CallServerGroupMeta(&req)
	if err != nil {
		w.Write([]byte(err.Error()))
	}
	if serverGroupMetaResp.ErrCode != pb.ErrCode_NO_ERR {
		w.Write([]byte(err.Error()))
	}
	fmt.Fprintln(w, "upload ok!")
}

func main() {
	http.HandleFunc("/", index)
	http.HandleFunc("/upload", upload)
	log.MainLogger.Info().Msgf("dashboard server success listen on: %s", ":12008")
	http.ListenAndServe(":12008", nil)
}
