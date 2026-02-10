package main

import (
	"context"
	"fmt"
	"log"
	"time"

	"google.golang.org/grpc"
	"google.golang.org/grpc/credentials/insecure"

	ctrlpb "github.com/eraft-io/eraft/shardctrlerpb"
	pb "github.com/eraft-io/eraft/shardkvpb"
)

// Collector collects cluster status from various nodes
type Collector struct {
	configAddrs []string
}

// NewCollector creates a new collector
func NewCollector(configAddrs []string) *Collector {
	return &Collector{
		configAddrs: configAddrs,
	}
}

// CollectDashboardData collects all dashboard data
func (c *Collector) CollectDashboardData() (*DashboardResponse, error) {
	now := time.Now()

	// Query latest config from ShardCtrler to get topology
	config, err := c.queryLatestConfig()
	if err != nil {
		return nil, fmt.Errorf("failed to query config: %w", err)
	}

	// Collect config cluster status
	configStatus, err := c.collectConfigStatus()
	if err != nil {
		return nil, fmt.Errorf("failed to collect config status: %w", err)
	}
	configStatus.ConfigNum = int(config.Num)

	// Collect shard groups status based on config
	shardGroups, shardMap, allShards, err := c.collectShardGroupsStatusFromConfig(config)
	if err != nil {
		return nil, fmt.Errorf("failed to collect shard groups status: %w", err)
	}

	// Build topology
	topology := ClusterTopology{
		ConfigCluster: *configStatus,
		ShardGroups:   shardGroups,
		ShardMap:      shardMap,
		TotalShards:   10, // eRaft default 10 shards
		UpdatedAt:     now,
	}

	// Calculate metrics
	metrics := c.calculateMetrics(topology, allShards)

	return &DashboardResponse{
		Topology: topology,
		Metrics:  metrics,
		Shards:   allShards,
	}, nil
}

// queryLatestConfig queries the latest config from ShardCtrler
func (c *Collector) queryLatestConfig() (*ctrlpb.Config, error) {
	for _, addr := range c.configAddrs {
		conn, err := grpc.Dial(addr, grpc.WithTransportCredentials(insecure.NewCredentials()))
		if err != nil {
			continue
		}
		defer conn.Close()

		client := ctrlpb.NewShardCtrlerServiceClient(conn)
		ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
		defer cancel()

		// Query with -1 to get latest config
		resp, err := client.Command(ctx, &ctrlpb.CommandRequest{
			Op:        ctrlpb.Op_QUERY,
			Num:       -1,
			ClientId:  time.Now().UnixNano(),
			CommandId: time.Now().UnixNano(),
		})
		if err != nil {
			log.Printf("Query config from %s failed: %v", addr, err)
			continue
		}

		// OK or empty string means success
		if resp.Err != "" && resp.Err != "OK" {
			log.Printf("Query config from %s returned error: %s", addr, resp.Err)
			continue
		}

		if resp.Config != nil {
			log.Printf("Successfully queried config from %s: ConfigNum=%d, Groups=%d", addr, resp.Config.Num, len(resp.Config.Groups))
			return resp.Config, nil
		}
	}

	return nil, fmt.Errorf("failed to query config from any ShardCtrler node")
}

// collectConfigStatus collects status from config cluster
func (c *Collector) collectConfigStatus() (*ConfigStatus, error) {
	nodes := make([]NodeStatus, 0, len(c.configAddrs))
	var leaderID int
	var totalStorage int64

	for i, addr := range c.configAddrs {
		conn, err := grpc.Dial(addr, grpc.WithTransportCredentials(insecure.NewCredentials()))
		if err != nil {
			nodes = append(nodes, NodeStatus{
				ID:      i,
				Address: addr,
				Healthy: false,
			})
			continue
		}
		defer conn.Close()

		client := ctrlpb.NewShardCtrlerServiceClient(conn)
		ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
		defer cancel()

		resp, err := client.GetStatus(ctx, &ctrlpb.GetStatusRequest{})
		if err != nil {
			nodes = append(nodes, NodeStatus{
				ID:      i,
				Address: addr,
				Healthy: false,
			})
			continue
		}

		role := RoleFollower
		if resp.State == "Leader" {
			role = RoleLeader
			leaderID = i
		}

		node := NodeStatus{
			ID:           i,
			Address:      addr,
			Role:         role,
			Term:         int(resp.Term),
			CommitIndex:  int(resp.CommitIndex),
			LastApplied:  int(resp.LastApplied),
			StorageBytes: resp.StorageSize,
			Healthy:      true,
			LastSeen:     time.Now(),
		}
		nodes = append(nodes, node)
		totalStorage += resp.StorageSize
	}

	return &ConfigStatus{
		ConfigNum:    0, // Will be updated from latest config
		Nodes:        nodes,
		LeaderID:     leaderID,
		TotalStorage: totalStorage,
	}, nil
}

// collectShardGroupsStatusFromConfig collects status from shard groups based on config
func (c *Collector) collectShardGroupsStatusFromConfig(config *ctrlpb.Config) ([]GroupStatus, map[int]int, []ShardStatus, error) {
	groups := make([]GroupStatus, 0)
	shardMap := make(map[int]int)
	allShards := make([]ShardStatus, 0)

	// Build shard map from config (shard index -> groupID)
	for shardID, gid := range config.Shards {
		shardMap[shardID] = int(gid)
	}

	// Iterate through all groups in config
	for gid, servers := range config.Groups {
		groupID := int(gid)
		addrs := servers.Servers
		if len(addrs) == 0 {
			continue
		}

		nodes := make([]NodeStatus, 0, len(addrs))
		var leaderID int
		var totalStorage int64
		var groupShards []int
		var leaderShardInfo map[int]*pb.ShardInfo // 保存Leader节点的shard信息

		// Find which shards belong to this group
		for shardID, assignedGID := range shardMap {
			if assignedGID == groupID {
				groupShards = append(groupShards, shardID)
			}
		}

		// Collect status from each node in the group
		for i, addr := range addrs {
			conn, err := grpc.Dial(addr, grpc.WithTransportCredentials(insecure.NewCredentials()))
			if err != nil {
				nodes = append(nodes, NodeStatus{
					ID:      i,
					Address: addr,
					Healthy: false,
				})
				continue
			}
			defer conn.Close()

			client := pb.NewShardKVServiceClient(conn)
			ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
			defer cancel()

			resp, err := client.GetStatus(ctx, &pb.GetStatusRequest{})
			if err != nil {
				nodes = append(nodes, NodeStatus{
					ID:      i,
					Address: addr,
					Healthy: false,
				})
				continue
			}

			role := RoleFollower
			if resp.State == "Leader" {
				role = RoleLeader
				leaderID = i
				// 从Leader节点保存shard信息
				if len(resp.Shards) > 0 {
					leaderShardInfo = make(map[int]*pb.ShardInfo)
					for _, shardInfo := range resp.Shards {
						leaderShardInfo[int(shardInfo.ShardId)] = shardInfo
					}
				}
			}

			node := NodeStatus{
				ID:           i,
				Address:      addr,
				Role:         role,
				Term:         int(resp.Term),
				CommitIndex:  int(resp.CommitIndex),
				LastApplied:  int(resp.LastApplied),
				StorageBytes: resp.StorageSize,
				Healthy:      true,
				LastSeen:     time.Now(),
			}
			nodes = append(nodes, node)
			totalStorage += resp.StorageSize
		}

		// Create shard status for each shard owned by this group
		for _, shardID := range groupShards {
			shard := ShardStatus{
				ShardID: shardID,
				GroupID: groupID,
				State:   "Serving", // Default state
				Keys:    0,
				Bytes:   0,
			}

			// 如果有Leader的shard信息，使用实际数据
			if leaderShardInfo != nil {
				if info, ok := leaderShardInfo[shardID]; ok {
					shard.State = info.Status
					shard.Keys = int(info.Keys)
					shard.Bytes = info.Bytes
				}
			}

			allShards = append(allShards, shard)
		}

		group := GroupStatus{
			GroupID:      groupID,
			Nodes:        nodes,
			Shards:       groupShards,
			TotalStorage: totalStorage,
			LeaderID:     leaderID,
		}
		groups = append(groups, group)
	}

	return groups, shardMap, allShards, nil
}

// calculateMetrics calculates aggregated metrics
func (c *Collector) calculateMetrics(topology ClusterTopology, shards []ShardStatus) MetricsData {
	totalNodes := len(topology.ConfigCluster.Nodes)
	healthyNodes := 0
	totalStorage := topology.ConfigCluster.TotalStorage
	totalKeys := 0

	for _, node := range topology.ConfigCluster.Nodes {
		if node.Healthy {
			healthyNodes++
		}
	}

	for _, group := range topology.ShardGroups {
		totalNodes += len(group.Nodes)
		totalStorage += group.TotalStorage
		for _, node := range group.Nodes {
			if node.Healthy {
				healthyNodes++
			}
		}
	}

	for _, shard := range shards {
		totalKeys += shard.Keys
	}

	averageLoad := 0.0
	if len(topology.ShardGroups) > 0 {
		averageLoad = float64(topology.TotalShards) / float64(len(topology.ShardGroups))
	}

	return MetricsData{
		TotalNodes:   totalNodes,
		HealthyNodes: healthyNodes,
		TotalStorage: totalStorage,
		TotalKeys:    totalKeys,
		TotalShards:  topology.TotalShards,
		AverageLoad:  averageLoad,
		UpdatedAt:    time.Now(),
	}
}
