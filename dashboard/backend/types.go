package main

import "time"

// NodeRole represents the role of a Raft node
type NodeRole string

const (
	RoleLeader    NodeRole = "Leader"
	RoleFollower  NodeRole = "Follower"
	RoleCandidate NodeRole = "Candidate"
)

// NodeStatus represents the status of a single node
type NodeStatus struct {
	ID           int       `json:"id"`
	Address      string    `json:"address"`
	Role         NodeRole  `json:"role"`
	Term         int       `json:"term"`
	CommitIndex  int       `json:"commitIndex"`
	LastApplied  int       `json:"lastApplied"`
	StorageBytes int64     `json:"storageBytes"`
	Healthy      bool      `json:"healthy"`
	LastSeen     time.Time `json:"lastSeen"`
}

// ShardStatus represents the status of a shard
type ShardStatus struct {
	ShardID int    `json:"shardId"`
	GroupID int    `json:"groupId"`
	State   string `json:"state"` // Serving, Pulling, BePulling, GCing
	Keys    int    `json:"keys"`
	Bytes   int64  `json:"bytes"`
}

// GroupStatus represents a shard group
type GroupStatus struct {
	GroupID      int          `json:"groupId"`
	Nodes        []NodeStatus `json:"nodes"`
	Shards       []int        `json:"shards"`
	TotalStorage int64        `json:"totalStorage"`
	LeaderID     int          `json:"leaderId"`
}

// ConfigStatus represents the configuration cluster status
type ConfigStatus struct {
	ConfigNum    int          `json:"configNum"`
	Nodes        []NodeStatus `json:"nodes"`
	LeaderID     int          `json:"leaderId"`
	TotalStorage int64        `json:"totalStorage"`
}

// ClusterTopology represents the overall cluster topology
type ClusterTopology struct {
	ConfigCluster ConfigStatus  `json:"configCluster"`
	ShardGroups   []GroupStatus `json:"shardGroups"`
	ShardMap      map[int]int   `json:"shardMap"` // shardId -> groupId
	TotalShards   int           `json:"totalShards"`
	UpdatedAt     time.Time     `json:"updatedAt"`
}

// MetricsData represents aggregated metrics
type MetricsData struct {
	TotalNodes   int       `json:"totalNodes"`
	HealthyNodes int       `json:"healthyNodes"`
	TotalStorage int64     `json:"totalStorage"`
	TotalKeys    int       `json:"totalKeys"`
	TotalShards  int       `json:"totalShards"`
	AverageLoad  float64   `json:"averageLoad"`
	UpdatedAt    time.Time `json:"updatedAt"`
}

// DashboardResponse wraps all dashboard data
type DashboardResponse struct {
	Topology ClusterTopology `json:"topology"`
	Metrics  MetricsData     `json:"metrics"`
	Shards   []ShardStatus   `json:"shards"`
}
