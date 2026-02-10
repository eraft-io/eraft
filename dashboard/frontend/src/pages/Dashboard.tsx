import React, { useState, useEffect } from 'react';
import { Layout, Tabs, Card, Spin, Alert, Space } from 'antd';
import { DashboardOutlined, ReloadOutlined } from '@ant-design/icons';
import { ClusterTopology } from '../components/ClusterTopology';
import { ShardView } from '../components/ShardView';
import { MetricsView } from '../components/MetricsView';
import { fetchDashboardData } from '../services/api';
import { DashboardResponse } from '../types';

const { Header, Content } = Layout;
const { TabPane } = Tabs;

export const Dashboard: React.FC = () => {
  const [data, setData] = useState<DashboardResponse | null>(null);
  const [loading, setLoading] = useState<boolean>(true);
  const [error, setError] = useState<string | null>(null);

  const loadData = async () => {
    try {
      setLoading(true);
      setError(null);
      const response = await fetchDashboardData();
      setData(response);
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Failed to load dashboard data');
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadData();
    const interval = setInterval(loadData, 5000); // Refresh every 5 seconds
    return () => clearInterval(interval);
  }, []);

  return (
    <Layout style={{ minHeight: '100vh' }}>
      <Header style={{ background: '#001529', padding: '0 24px' }}>
        <Space style={{ color: 'white', fontSize: 20, fontWeight: 'bold' }}>
          <DashboardOutlined />
          <span>eRaft Dashboard</span>
          <ReloadOutlined
            onClick={loadData}
            style={{ marginLeft: 20, fontSize: 16, cursor: 'pointer' }}
          />
        </Space>
      </Header>
      <Content style={{ padding: '24px' }}>
        {loading && !data ? (
          <Card>
            <Spin size="large" />
          </Card>
        ) : error ? (
          <Alert
            message="Error"
            description={error}
            type="error"
            showIcon
          />
        ) : data ? (
          <Tabs defaultActiveKey="1">
            <TabPane tab="Metrics Overview" key="1">
              <MetricsView metrics={data.metrics} />
            </TabPane>
            <TabPane tab="Cluster Topology" key="2">
              <ClusterTopology topology={data.topology} />
            </TabPane>
            <TabPane tab="Shard Status" key="3">
              <ShardView shards={data.shards} shardMap={data.topology.shardMap} />
            </TabPane>
          </Tabs>
        ) : null}
        {data && (
          <Card size="small" style={{ marginTop: 16 }}>
            <div style={{ fontSize: 12, color: '#999' }}>
              Last updated: {new Date(data.metrics.updatedAt).toLocaleString()}
            </div>
          </Card>
        )}
      </Content>
    </Layout>
  );
};
