import axios from 'axios';
import { DashboardResponse, ClusterTopology, MetricsData } from '../types';

const API_BASE_URL = process.env.REACT_APP_API_BASE_URL || 'http://localhost:8080/api';

const apiClient = axios.create({
  baseURL: API_BASE_URL,
  timeout: 10000,
});

export const fetchDashboardData = async (): Promise<DashboardResponse> => {
  const response = await apiClient.get<DashboardResponse>('/dashboard');
  return response.data;
};

export const fetchTopology = async (): Promise<ClusterTopology> => {
  const response = await apiClient.get<ClusterTopology>('/topology');
  return response.data;
};

export const fetchMetrics = async (): Promise<MetricsData> => {
  const response = await apiClient.get<MetricsData>('/metrics');
  return response.data;
};
