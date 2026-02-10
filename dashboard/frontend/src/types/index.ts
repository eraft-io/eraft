export type NodeRole = 'Leader' | 'Follower' | 'Candidate';

export interface NodeStatus {
  id: number;
  address: string;
  role: NodeRole;
  term: number;
  commitIndex: number;
  lastApplied: number;
  storageBytes: number;
  healthy: boolean;
  lastSeen: string;
}

export interface ShardStatus {
  shardId: number;
  groupId: number;
  state: string;
  keys: number;
  bytes: number;
}

export interface GroupStatus {
  groupId: number;
  nodes: NodeStatus[];
  shards: number[];
  totalStorage: number;
  leaderId: number;
}

export interface ConfigStatus {
  configNum: number;
  nodes: NodeStatus[];
  leaderId: number;
  totalStorage: number;
}

export interface ClusterTopology {
  configCluster: ConfigStatus;
  shardGroups: GroupStatus[];
  shardMap: { [key: number]: number };
  totalShards: number;
  updatedAt: string;
}

export interface MetricsData {
  totalNodes: number;
  healthyNodes: number;
  totalStorage: number;
  totalKeys: number;
  totalShards: number;
  averageLoad: number;
  updatedAt: string;
}

export interface DashboardResponse {
  topology: ClusterTopology;
  metrics: MetricsData;
  shards: ShardStatus[];
}
