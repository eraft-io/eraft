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
<title>WellWood</title>
</head>
<body>

<pre> 
_       __     _____       __                __
| |     / /__  / / / |     / /___  ____  ____/ /
| | /| / / _ \/ / /| | /| / / __ \/ __ \/ __  /
| |/ |/ /  __/ / / | |/ |/ / /_/ / /_/ / /_/ /
|__/|__/\___/_/_/  |__/|__/\____/\____/\__,_/
</pre>

<h4>选择要上传到 wellwood 对象存储系统的文件</h4>
<form enctype="multipart/form-data" action="/upload" method="post">
 <input type="file" name="uploadfile" />
 <input type="hidden" name="token" value="82e9fd7deb40aaedd32641af0512abbc"/>
 <input type="submit" value="upload" />
</form>
`

var metaNodeAddrs = flag.String("meta_addrs", "wellwood-metaserver-0.wellwood-metaserver:8088,wellwood-metaserver-1.wellwood-metaserver:8089,wellwood-metaserver-2.wellwood-metaserver:8090", "input block server node addrs")

func index(w http.ResponseWriter, r *http.Request) {
	w.Write([]byte(tpl))
	c := clientsdk.NewClient(*metaNodeAddrs, "", "")
	// query server group meta
	req := pb.ServerGroupMetaConfigRequest{
		BucketOpReq: &pb.BucketOpRequest{
			BucketId: consts.DEFAULT_BUCKET_ID,
		},
		OpType: pb.ConfigServerGroupMetaOpType_OP_OSS_OBJECT_LIST,
	}
	serverGroupMetaResp := c.GetMetaSvrCli().CallServerGroupMeta(&req)
	if serverGroupMetaResp != nil {
		w.Write([]byte(`<a href="javascript:location.reload();">点击刷新列表</a>`))
		w.Write([]byte(`<h4>对象列表</h4> <table border="1"> <tr> <th>对象 ID</th> <th>对象名</th> <th>操作</th> </tr>`))
		for _, obj := range serverGroupMetaResp.BucketOpRes.Objects {
			w.Write([]byte("<tr>"))
			w.Write([]byte("<td>"))
			w.Write([]byte(obj.ObjectName))
			w.Write([]byte("</td>"))
			w.Write([]byte("<td>"))
			w.Write([]byte(obj.ObjectId))
			w.Write([]byte("</td>"))
			w.Write([]byte("<td>"))
			w.Write([]byte(`<a target="_blank" href="/download?obj_id=` + obj.ObjectName + `">点击下载</a>`))
			w.Write([]byte("</td>"))
			w.Write([]byte("</tr>"))
		}
	}
	w.Write([]byte(`</body>
	</html>`))
}

func download(w http.ResponseWriter, r *http.Request) {
	objId, ok := r.URL.Query()["obj_id"]
	if !ok || len(objId[0]) < 1 {
		log.MainLogger.Error().Msgf("Url Param 'obj_id' is missing")
		return
	}
	req := pb.ServerGroupMetaConfigRequest{
		ConfigVersion: -1,
		OpType:        pb.ConfigServerGroupMetaOpType_OP_OSS_OBJECT_LIST,
		BucketOpReq: &pb.BucketOpRequest{
			BucketId: consts.DEFAULT_BUCKET_ID,
		},
	}
	c := clientsdk.NewClient(*metaNodeAddrs, "", "")
	objListMetaResp := c.GetMetaSvrCli().CallServerGroupMeta(&req)
	if objListMetaResp.ErrCode != pb.ErrCode_NO_ERR {
		log.MainLogger.Error().Msgf("got objects meta error")
	}
	w.WriteHeader(http.StatusOK)
	w.Header().Set("Content-Type", "application/octet-stream")
	for _, obj := range objListMetaResp.BucketOpRes.Objects {
		if obj.ObjectName == objId[0] {
			// find block meta
			// query server group meta
			req := pb.ServerGroupMetaConfigRequest{
				ConfigVersion: -1,
				OpType:        pb.ConfigServerGroupMetaOpType_OP_SERVER_GROUP_QUERY,
			}
			serverGroupMetaResp := c.GetMetaSvrCli().CallServerGroupMeta(&req)
			slotsToGroupArr := serverGroupMetaResp.ServerGroupMetas.Slots
			for _, blockMeta := range obj.ObjectBlocksMeta {
				serverGroupAddrs := serverGroupMetaResp.ServerGroupMetas.ServerGroups[slotsToGroupArr[blockMeta.BlockSlotId]]
				// find block server
				serverAddrArr := strings.Split(serverGroupAddrs, ",")
				blockSvrCli := blockserver.MakeBlockServerClient(serverAddrArr)
				fileBlockRequest := &pb.FileBlockOpRequest{
					FileName:       objId[0],
					FileBlocksMeta: blockMeta,
					OpType:         pb.FileBlockOpType_OP_BLOCK_READ,
				}
				readBlockResp := blockSvrCli.CallFileBlockOp(fileBlockRequest)
				w.Write(readBlockResp.BlockContent)
			}
		}
	}
}

func upload(w http.ResponseWriter, r *http.Request) {
	r.ParseMultipartForm(32 << 20)
	file, handler, err := r.FormFile("uploadfile")
	if err != nil {
		fmt.Println(err)
		return
	}
	defer file.Close()
	// upload to wellwood
	log.MainLogger.Debug().Msgf("meta addrs: %s", *metaNodeAddrs)
	c := clientsdk.NewClient(*metaNodeAddrs, "", "")
	fileReader := bufio.NewReader(file)
	blockSize := consts.FILE_BLOCK_SIZE
	if handler.Size > 1024*1024*50 && handler.Size <= 1024*1024*300 {
		blockSize = consts.FILE_BLOCK_SIZE_64MB
	}
	if handler.Size > 1024*1024*300 {
		blockSize = consts.FILE_BLOCK_SIZE_128MB
	}
	blockBuf := make([]byte, blockSize)
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
	if serverGroupMetaResp != nil {
		if serverGroupMetaResp.ErrCode != pb.ErrCode_NO_ERR {
			log.MainLogger.Error().Msgf("%d", serverGroupMetaResp.ErrCode)
		}
	}
	w.Write([]byte(`<html>
	<head>
	<title>上传成功</title>
	</head>
	<body>
	`))
	w.Write([]byte(`<a href="javascript:history.go(-1);">返回继续传文件</a> <body> </html>`))
}

func main() {
	flag.Parse()
	http.HandleFunc("/", index)
	http.HandleFunc("/upload", upload)
	http.HandleFunc("/download", download)
	log.MainLogger.Info().Msgf("dashboard server success listen on: %s", ":12008")
	http.ListenAndServe(":12008", nil)
}
