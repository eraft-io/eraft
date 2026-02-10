import React from 'react';
import { Card, Tag, Space, Descriptions } from 'antd';
import {
  ClusterOutlined,
  DatabaseOutlined,
  CheckCircleOutlined,
  CloseCircleOutlined,
  CrownOutlined,
} from '@ant-design/icons';
import { ClusterTopology as ClusterTopologyType, NodeStatus } from '../types';

interface ClusterTopologyProps {
  topology: ClusterTopologyType;
}

const getRoleColor = (role: string): string => {
  switch (role) {
    case 'Leader':
      return 'gold';
    case 'Follower':
      return 'blue';
    case 'Candidate':
      return 'orange';
    default:
      return 'default';
  }
};

const formatBytes = (bytes: number): string => {
  if (bytes === 0) return '0 B';
  const k = 1024;
  const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return Math.round((bytes / Math.pow(k, i)) * 100) / 100 + ' ' + sizes[i];
};

const NodeCard: React.FC<{ node: NodeStatus; isLeader: boolean }> = ({ node, isLeader }) => (
  <Card
    size="small"
    style={{
      marginBottom: 8,
      borderColor: isLeader ? '#faad14' : undefined,
      borderWidth: isLeader ? 2 : 1,
    }}
  >
    <Space direction="vertical" size="small" style={{ width: '100%' }}>
      <Space>
        {isLeader && <CrownOutlined style={{ color: '#faad14' }} />}
        <strong>Node {node.id}</strong>
        <Tag color={getRoleColor(node.role)}>{node.role}</Tag>
        {node.healthy ? (
          <CheckCircleOutlined style={{ color: '#52c41a' }} />
        ) : (
          <CloseCircleOutlined style={{ color: '#ff4d4f' }} />
        )}
      </Space>
      <div style={{ fontSize: 12 }}>
        <div>Address: {node.address}</div>
        <div>Term: {node.term} | Commit: {node.commitIndex} | Applied: {node.lastApplied}</div>
        <div>Storage: {formatBytes(node.storageBytes)}</div>
      </div>
    </Space>
  </Card>
);

export const ClusterTopology: React.FC<ClusterTopologyProps> = ({ topology }) => {
  return (
    <div>
      <Card
        title={
          <Space>
            <ClusterOutlined />
            <span>Configuration Cluster</span>
          </Space>
        }
        style={{ marginBottom: 16 }}
      >
        <Descriptions size="small" column={3} style={{ marginBottom: 16 }}>
          <Descriptions.Item label="Config Number">
            {topology.configCluster.configNum}
          </Descriptions.Item>
          <Descriptions.Item label="Total Storage">
            {formatBytes(topology.configCluster.totalStorage)}
          </Descriptions.Item>
          <Descriptions.Item label="Leader">
            Node {topology.configCluster.leaderId}
          </Descriptions.Item>
        </Descriptions>
        {topology.configCluster.nodes.map((node) => (
          <NodeCard
            key={node.id}
            node={node}
            isLeader={node.id === topology.configCluster.leaderId}
          />
        ))}
      </Card>

      {topology.shardGroups.map((group) => (
        <Card
          key={group.groupId}
          title={
            <Space>
              <DatabaseOutlined />
              <span>Shard Group {group.groupId}</span>
              <Tag>Shards: {group.shards.join(', ')}</Tag>
            </Space>
          }
          style={{ marginBottom: 16 }}
        >
          <Descriptions size="small" column={2} style={{ marginBottom: 16 }}>
            <Descriptions.Item label="Total Storage">
              {formatBytes(group.totalStorage)}
            </Descriptions.Item>
            <Descriptions.Item label="Leader">
              Node {group.leaderId}
            </Descriptions.Item>
          </Descriptions>
          {group.nodes.map((node) => (
            <NodeCard
              key={node.id}
              node={node}
              isLeader={node.id === group.leaderId}
            />
          ))}
        </Card>
      ))}
    </div>
  );
};
