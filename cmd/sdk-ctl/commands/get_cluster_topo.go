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

package commands

import (
	"encoding/json"
	"fmt"
	"strings"

	meta_svr "github.com/eraft-io/eraft/pkg/metaserver"
	pb "github.com/eraft-io/eraft/pkg/protocol"
	"github.com/tidwall/pretty"

	"github.com/spf13/cobra"
)

var getClusterTopoCmd = &cobra.Command{
	Use:   "get_cluster_topo [meta server addrs]",
	Short: "get cluster topology for wellwood cluster",
	Long:  ``,
	Run: func(cmd *cobra.Command, args []string) {
		metaServerAddrArr := strings.Split(args[0], ",")
		metaCli := meta_svr.MakeMetaServerClient(metaServerAddrArr)
		req := &pb.ServerGroupMetaConfigRequest{
			ConfigVersion: -1,
			OpType:        pb.ConfigServerGroupMetaOpType_OP_SERVER_GROUP_QUERY,
		}
		resp := metaCli.CallServerGroupMeta(req)
		data, _ := json.Marshal(resp)
		var Options = &pretty.Options{Width: 80, Prefix: "", Indent: "\t", SortKeys: false}
		fmt.Printf("%s\n", pretty.Color(pretty.PrettyOptions(data, Options), pretty.TerminalStyle))
	},
}

func init() {
	rootCmd.AddCommand(getClusterTopoCmd)
}
