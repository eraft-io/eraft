import React from 'react';
import { Card, Table, Tag, Space } from 'antd';
import { AppstoreOutlined } from '@ant-design/icons';
import { ShardStatus } from '../types';

interface ShardViewProps {
  shards: ShardStatus[];
  shardMap: { [key: number]: number };
}

const formatBytes = (bytes: number): string => {
  if (bytes === 0) return '0 B';
  const k = 1024;
  const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return Math.round((bytes / Math.pow(k, i)) * 100) / 100 + ' ' + sizes[i];
};

const getStateColor = (state: string): string => {
  switch (state) {
    case 'Serving':
      return 'success';
    case 'Pulling':
      return 'processing';
    case 'BePulling':
      return 'warning';
    case 'GCing':
      return 'default';
    default:
      return 'default';
  }
};

export const ShardView: React.FC<ShardViewProps> = ({ shards, shardMap }) => {
  const columns = [
    {
      title: 'Shard ID',
      dataIndex: 'shardId',
      key: 'shardId',
      sorter: (a: ShardStatus, b: ShardStatus) => a.shardId - b.shardId,
    },
    {
      title: 'Group ID',
      dataIndex: 'groupId',
      key: 'groupId',
      sorter: (a: ShardStatus, b: ShardStatus) => a.groupId - b.groupId,
    },
    {
      title: 'State',
      dataIndex: 'state',
      key: 'state',
      render: (state: string) => (
        <Tag color={getStateColor(state)}>{state}</Tag>
      ),
      filters: [
        { text: 'Serving', value: 'Serving' },
        { text: 'Pulling', value: 'Pulling' },
        { text: 'BePulling', value: 'BePulling' },
        { text: 'GCing', value: 'GCing' },
      ],
      onFilter: (value: unknown, record: ShardStatus) => record.state === value,
    },
    {
      title: 'Keys',
      dataIndex: 'keys',
      key: 'keys',
      sorter: (a: ShardStatus, b: ShardStatus) => a.keys - b.keys,
    },
    {
      title: 'Storage',
      dataIndex: 'bytes',
      key: 'bytes',
      render: (bytes: number) => formatBytes(bytes),
      sorter: (a: ShardStatus, b: ShardStatus) => a.bytes - b.bytes,
    },
  ];

  // Create shard distribution visualization data
  const shardDistribution = Object.entries(shardMap).reduce((acc, [shardId, groupId]) => {
    acc[groupId] = (acc[groupId] || 0) + 1;
    return acc;
  }, {} as { [key: number]: number });

  return (
    <div>
      <Card
        title={
          <Space>
            <AppstoreOutlined />
            <span>Shard Distribution</span>
          </Space>
        }
        style={{ marginBottom: 16 }}
      >
        <Space>
          {Object.entries(shardDistribution).map(([groupId, count]) => (
            <Tag key={groupId} color="blue">
              Group {groupId}: {count} shards
            </Tag>
          ))}
        </Space>
      </Card>

      <Card
        title={
          <Space>
            <AppstoreOutlined />
            <span>Shard Details</span>
          </Space>
        }
      >
        <Table
          dataSource={shards}
          columns={columns}
          rowKey="shardId"
          size="small"
          pagination={{ pageSize: 20 }}
        />
      </Card>
    </div>
  );
};
