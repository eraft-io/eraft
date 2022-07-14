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

	"github.com/eraft-io/eraft/pkg/clientsdk"
	"github.com/spf13/cobra"
	"github.com/tidwall/pretty"
)

var getObjectCmd = &cobra.Command{
	Use:   "get_object [meta server addrs] [bucket id] [object id] [local path]",
	Short: "get object from wellwood cluster to local",
	Long:  ``,
	Run: func(cmd *cobra.Command, args []string) {
		wellWoodCli := clientsdk.NewClient(args[0], "", "")
		if err := wellWoodCli.DownloadFile(args[1], args[2], args[3]); err != nil {
			data, _ := json.Marshal(err.Error())
			var Options = &pretty.Options{Width: 80, Prefix: "", Indent: "\t", SortKeys: false}
			fmt.Printf("%s\n", pretty.Color(pretty.PrettyOptions(data, Options), pretty.TerminalStyle))
		}
	},
}

func init() {
	rootCmd.AddCommand(getObjectCmd)
}
