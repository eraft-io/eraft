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

package clientsdk

import (
	"bufio"
	"context"
	"errors"
	"io"
	"os"
	"strings"

	block_server "github.com/eraft-io/eraft/pkg/blockserver"
	common "github.com/eraft-io/eraft/pkg/common"
	"github.com/eraft-io/eraft/pkg/consts"
	meta_server "github.com/eraft-io/eraft/pkg/metaserver"

	pb "github.com/eraft-io/eraft/pkg/protocol"
)

type ClientSdk struct {
	metaSvrCli  *meta_server.MetaServerClientEnd
	blockSvcCli *block_server.BlockServerClientEnd
}

func (c *ClientSdk) UploadFile(localPath string) error {
	f, err := os.Open(localPath)
	if err != nil {
		return err
	}
	fileReader := bufio.NewReader(f)
	blockBuf := make([]byte, consts.FILE_BLOCK_SIZE)
	fileBlockMetas := []*pb.FileBlockMeta{}
	index := 0
	for {
		n, err := fileReader.Read(blockBuf)
		if err != nil && err != io.EOF {
			return err
		}
		// read last file block
		if n == 0 {
			break
		}
		// query server group meta
		serverGroupMetaReq := pb.ServerGroupMetaConfigRequest{
			OpType: pb.ConfigServerGroupMetaOpType_OP_SERVER_GROUP_QUERY,
		}
		serverGroupMetaResp, err := c.metaSvrCli.GetMetaSvrCli().ServerGroupMeta(context.Background(), &serverGroupMetaReq)
		if err != nil {
			return err
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
		c.blockSvcCli = block_server.MakeBlockServerClient(serverAddrArr[0])
		fileBlockRequest := pb.WriteFileBlockRequest{
			FileName:       localPath,
			FileBlocksMeta: blockMeta,
			BlockContent:   blockBuf[:n],
		}
		writeBlockResp, err := c.blockSvcCli.GetBlockSvrCli().WriteFileBlock(context.Background(), &fileBlockRequest)
		if err != nil {
			return err
		}
		if writeBlockResp.ErrCode != pb.ErrCode_NO_ERR {
			return errors.New("")
		}
		if writeBlockResp.ErrCode == pb.ErrCode_WRONG_LEADER_ERR {
			c.blockSvcCli = block_server.MakeBlockServerClient(serverAddrArr[writeBlockResp.LeaderId])
			fileBlockRequest := pb.WriteFileBlockRequest{
				FileName:       localPath,
				FileBlocksMeta: blockMeta,
				BlockContent:   blockBuf[:n],
			}
			writeBlockResp, err := c.blockSvcCli.GetBlockSvrCli().WriteFileBlock(context.Background(), &fileBlockRequest)
			if err != nil {
				return err
			}
			if writeBlockResp.ErrCode != pb.ErrCode_NO_ERR {
				return errors.New("")
			}
		}
		fileBlockMetas = append(fileBlockMetas, blockMeta)
		index += 1
	}
	fileBlockMetaConfigRequest := &pb.FileBlockMetaConfigRequest{
		OpType:         pb.FileBlockMetaConfigOpType_OP_ADD_FILE_BLOCK_META,
		FileBlocksMeta: fileBlockMetas,
	}
	fileBlockMetasWriteResp, err := c.metaSvrCli.GetMetaSvrCli().FileBlockMeta(context.Background(), fileBlockMetaConfigRequest)
	if err != nil {
		return err
	}
	if fileBlockMetasWriteResp.ErrCode != pb.ErrCode_NO_ERR {
		return errors.New("")
	}
	return nil
}

func (c *ClientSdk) DownloadFile(path string) ([]byte, error) {
	//1.FileBlockMeta find file block meta

	//2.call ServerGroupMeta query with servers

	//3.call ReadFileBlock to read all file blocks
	return nil, nil
}
