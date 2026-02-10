import React from 'react';
import { Card, Row, Col, Statistic, Progress } from 'antd';
import {
  ClusterOutlined,
  DatabaseOutlined,
  KeyOutlined,
  CheckCircleOutlined,
} from '@ant-design/icons';
import ReactECharts from 'echarts-for-react';
import { MetricsData } from '../types';

interface MetricsViewProps {
  metrics: MetricsData;
}

const formatBytes = (bytes: number): string => {
  if (bytes === 0) return '0 B';
  const k = 1024;
  const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return Math.round((bytes / Math.pow(k, i)) * 100) / 100 + ' ' + sizes[i];
};

export const MetricsView: React.FC<MetricsViewProps> = ({ metrics }) => {
  const healthRate = metrics.totalNodes > 0
    ? (metrics.healthyNodes / metrics.totalNodes) * 100
    : 0;

  const storageChartOption = {
    title: {
      text: 'Storage Overview',
      left: 'center',
    },
    tooltip: {
      trigger: 'item',
      formatter: '{b}: {c} ({d}%)',
    },
    series: [
      {
        name: 'Storage',
        type: 'pie',
        radius: '50%',
        data: [
          { value: metrics.totalStorage, name: formatBytes(metrics.totalStorage) },
        ],
        emphasis: {
          itemStyle: {
            shadowBlur: 10,
            shadowOffsetX: 0,
            shadowColor: 'rgba(0, 0, 0, 0.5)',
          },
        },
      },
    ],
  };

  const shardDistributionOption = {
    title: {
      text: 'Shard Distribution',
      left: 'center',
    },
    tooltip: {
      trigger: 'axis',
      axisPointer: {
        type: 'shadow',
      },
    },
    xAxis: {
      type: 'category',
      data: ['Total Shards'],
    },
    yAxis: {
      type: 'value',
    },
    series: [
      {
        name: 'Shards',
        type: 'bar',
        data: [metrics.totalShards],
        itemStyle: {
          color: '#1890ff',
        },
      },
    ],
  };

  return (
    <div>
      <Row gutter={[16, 16]} style={{ marginBottom: 16 }}>
        <Col span={6}>
          <Card>
            <Statistic
              title="Total Nodes"
              value={metrics.totalNodes}
              prefix={<ClusterOutlined />}
            />
          </Card>
        </Col>
        <Col span={6}>
          <Card>
            <Statistic
              title="Healthy Nodes"
              value={metrics.healthyNodes}
              prefix={<CheckCircleOutlined />}
              valueStyle={{ color: '#3f8600' }}
            />
            <Progress percent={Math.round(healthRate)} size="small" />
          </Card>
        </Col>
        <Col span={6}>
          <Card>
            <Statistic
              title="Total Storage"
              value={formatBytes(metrics.totalStorage)}
              prefix={<DatabaseOutlined />}
            />
          </Card>
        </Col>
        <Col span={6}>
          <Card>
            <Statistic
              title="Total Keys"
              value={metrics.totalKeys}
              prefix={<KeyOutlined />}
            />
          </Card>
        </Col>
      </Row>

      <Row gutter={[16, 16]} style={{ marginBottom: 16 }}>
        <Col span={6}>
          <Card>
            <Statistic
              title="Total Shards"
              value={metrics.totalShards}
            />
          </Card>
        </Col>
        <Col span={6}>
          <Card>
            <Statistic
              title="Average Load"
              value={metrics.averageLoad.toFixed(2)}
              suffix="shards/group"
            />
          </Card>
        </Col>
      </Row>

      <Row gutter={[16, 16]}>
        <Col span={12}>
          <Card>
            <ReactECharts option={storageChartOption} style={{ height: '300px' }} />
          </Card>
        </Col>
        <Col span={12}>
          <Card>
            <ReactECharts option={shardDistributionOption} style={{ height: '300px' }} />
          </Card>
        </Col>
      </Row>
    </div>
  );
};
