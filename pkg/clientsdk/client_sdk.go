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
	"errors"
	"io"
	"os"
	"strings"

	"github.com/eraft-io/eraft/pkg/blockserver"
	"github.com/eraft-io/eraft/pkg/common"
	"github.com/eraft-io/eraft/pkg/consts"
	"github.com/eraft-io/eraft/pkg/metaserver"
	pb "github.com/eraft-io/eraft/pkg/protocol"
)

type Client struct {
	metaSvrCli  *metaserver.MetaSvrCli
	blockSvrCli *blockserver.BlockSvrCli
}

func NewClient(metaSvrAddrs string, accessKey string, accessSecret string) *Client {
	metaServerAddrArr := strings.Split(metaSvrAddrs, ",")
	metaCli := metaserver.MakeMetaServerClient(metaServerAddrArr)
	return &Client{
		metaSvrCli: metaCli,
	}
}

func (c *Client) ListBuckets() ([]*pb.Bucket, error) {
	return nil, nil
}

func (c *Client) CreateBucket(name string) error {
	return nil
}

func (c *Client) DeleteBucket() error {
	return nil
}

func (c *Client) Bucket(name string) (*pb.Bucket, error) {
	return nil, nil
}

func (c *Client) UploadFile(localPath string, bucketId string) (string, error) {
	f, err := os.Open(localPath)
	if err != nil {
		return "", err
	}
	fileReader := bufio.NewReader(f)
	blockBuf := make([]byte, consts.FILE_BLOCK_SIZE)
	fileBlockMetas := []*pb.FileBlockMeta{}
	objRandId := common.GenGoogleUUID()
	index := 0
	for {
		n, err := fileReader.Read(blockBuf)
		if err != nil && err != io.EOF {
			return "", err
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
		serverGroupMetaResp := c.metaSvrCli.CallServerGroupMeta(&req)
		if err != nil {
			return "", err
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
		c.blockSvrCli = blockserver.MakeBlockServerClient(serverAddrArr)
		fileBlockRequest := &pb.FileBlockOpRequest{
			FileName:       objRandId,
			FileBlocksMeta: blockMeta,
			BlockContent:   blockBuf[:n],
			OpType:         pb.FileBlockOpType_OP_BLOCK_WRITE,
		}
		writeBlockResp := c.blockSvrCli.CallFileBlockOp(fileBlockRequest)
		if err != nil {
			return "", err
		}
		if writeBlockResp.ErrCode != pb.ErrCode_NO_ERR {
			return "", errors.New("write error")
		}
		fileBlockMetas = append(fileBlockMetas, blockMeta)
		index += 1
	}
	// wirte obj meta to meta server
	req := pb.ServerGroupMetaConfigRequest{
		OpType: pb.ConfigServerGroupMetaOpType_OP_OSS_OBJECT_PUT,
		BucketOpReq: &pb.BucketOpRequest{
			Object: &pb.Object{
				ObjectId:         localPath,
				ObjectName:       objRandId,
				FromBucketId:     bucketId,
				ObjectBlocksMeta: fileBlockMetas,
			},
			BucketId: bucketId,
		},
	}
	serverGroupMetaResp := c.metaSvrCli.CallServerGroupMeta(&req)
	if err != nil {
		return "", err
	}
	if serverGroupMetaResp.ErrCode != pb.ErrCode_NO_ERR {
		return "", errors.New("write meta error")
	}
	return objRandId, nil
}

func (c *Client) DownloadFile(bucketId string, objName string, localFilePath string) error {
	//1.FileBlockMeta find file block meta
	// query object list
	req := pb.ServerGroupMetaConfigRequest{
		ConfigVersion: -1,
		OpType:        pb.ConfigServerGroupMetaOpType_OP_OSS_OBJECT_LIST,
		BucketOpReq: &pb.BucketOpRequest{
			BucketId: bucketId,
		},
	}
	objListMetaResp := c.metaSvrCli.CallServerGroupMeta(&req)
	if objListMetaResp.ErrCode != pb.ErrCode_NO_ERR {
		return errors.New("query object list err")
	}
	file, err := os.OpenFile(localFilePath, os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0666)
	if err != nil {
		return err
	}
	defer file.Close()
	writer := bufio.NewWriter(file)
	for _, obj := range objListMetaResp.BucketOpRes.Objects {
		if obj.ObjectName == objName {
			// find block meta
			// query server group meta
			req := pb.ServerGroupMetaConfigRequest{
				ConfigVersion: -1,
				OpType:        pb.ConfigServerGroupMetaOpType_OP_SERVER_GROUP_QUERY,
			}
			serverGroupMetaResp := c.metaSvrCli.CallServerGroupMeta(&req)
			slotsToGroupArr := serverGroupMetaResp.ServerGroupMetas.Slots
			for _, blockMeta := range obj.ObjectBlocksMeta {
				serverGroupAddrs := serverGroupMetaResp.ServerGroupMetas.ServerGroups[slotsToGroupArr[blockMeta.BlockSlotId]]
				// find block server
				serverAddrArr := strings.Split(serverGroupAddrs, ",")
				c.blockSvrCli = blockserver.MakeBlockServerClient(serverAddrArr)
				fileBlockRequest := &pb.FileBlockOpRequest{
					FileName:       objName,
					FileBlocksMeta: blockMeta,
					OpType:         pb.FileBlockOpType_OP_BLOCK_READ,
				}
				readBlockResp := c.blockSvrCli.CallFileBlockOp(fileBlockRequest)
				writer.Write(readBlockResp.BlockContent)
				writer.Flush()
			}
		}
	}
	return nil
}
