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
	"fmt"

	"github.com/eraft-io/eraft/pkg/clientsdk"
	"github.com/spf13/cobra"
)

var putObjectCmd = &cobra.Command{
	Use:   "put_object [meta server addrs] [local file path] [bucket id]",
	Short: "put object to remote wellwood cluster",
	Long:  ``,
	Run: func(cmd *cobra.Command, args []string) {
		wellWoodCli := clientsdk.NewClient(args[0], "", "")
		objectId, err := wellWoodCli.UploadFile(args[1], args[2])
		if err != nil {
			fmt.Printf("%s\n", err.Error())
		}
		fmt.Printf("put object return %s\n", objectId)
	},
}

func init() {
	rootCmd.AddCommand(putObjectCmd)
}
