package commands

import (
	"fmt"

	"github.com/spf13/cobra"
)

var metaSvrAddrs string

var getClusterTopoCmd = &cobra.Command{
	Use:   "get_cluster_topo",
	Short: "get cluster topology for wellwood cluster",
	Long:  ``,
	Run: func(cmd *cobra.Command, args []string) {
		fmt.Println(metaSvrAddrs)
	},
}

func init() {
	getClusterTopoCmd.Flags().StringVarP(&metaSvrAddrs, "meta_server_addrs", "", "127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090", "ip1,ip2,ip3")
	rootCmd.AddCommand(getClusterTopoCmd)
}
