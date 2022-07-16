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
	"path/filepath"
	"time"

	"github.com/eraft-io/eraft/pkg/clientsdk"
	"github.com/eraft-io/eraft/pkg/common"
	"github.com/eraft-io/eraft/pkg/consts"
	"github.com/spf13/cobra"
)

var simpleBenchCmd = &cobra.Command{
	Use:   "simple_bench [meta server addrs] [bench_files_dir]",
	Short: "simple benchmark for wellwood",
	Long:  ``,
	Run: func(cmd *cobra.Command, args []string) {
		metaSvrAddrs := args[0]
		benchFileDir := args[1]
		testBucketId := "3f8602ef-9488-419f-a485-4df0cbd73c3"
		benchFileNames, totalSize, err := common.ReadFileMetaInDir(benchFileDir)
		if err != nil {
			fmt.Println(err.Error())
		}
		wellWoodCli := clientsdk.NewClient(metaSvrAddrs, "", "")
		startT := time.Now()
		for _, fileName := range benchFileNames {
			wellWoodCli.UploadFile(filepath.Join(benchFileDir, fileName), testBucketId)
		}
		tc := time.Since(startT).Seconds() //计算耗时
		fmt.Printf("time cost = %f s\n", tc)
		fmt.Printf("speed = %f MB/s\n", float64(totalSize)/float64(consts.MB)/tc)
	},
}

func init() {
	rootCmd.AddCommand(simpleBenchCmd)
}
