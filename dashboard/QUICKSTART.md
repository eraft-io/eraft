# Dashboard 快速开始

## 1. 构建后端

```bash
# 在项目根目录执行
make builddashboard
```

这将构建 dashboard 后端服务器到 `output/dashboard-server`

## 2. 启动后端服务

```bash
# 方式1：使用快速启动脚本
cd dashboard
./start-dashboard.sh

# 方式2：直接运行
./output/dashboard-server \
  -port=8080 \
  -config-addrs="localhost:8001,localhost:8002,localhost:8003" \
  -shard-groups="100:localhost:9001,localhost:9002,localhost:9003;101:localhost:9004,localhost:9005,localhost:9006"
```

## 3. 测试 API

```bash
# 获取集群状态
curl http://localhost:8080/api/dashboard | jq

# 获取拓扑信息
curl http://localhost:8080/api/topology | jq

# 获取指标
curl http://localhost:8080/api/metrics | jq
```

## 4. 构建前端（可选）

如果需要 Web 界面，需要先安装前端依赖并构建：

**注意**：项目已配置淘宝 npm 镜像源（`https://registry.npmmirror.com`），加快国内依赖安装速度。

```bash
cd dashboard/frontend

# 安装依赖（首次需要，已自动使用淘宝镜像）
npm install

# 开发模式运行
npm start
# 然后访问 http://localhost:3000

# 或构建生产版本
npm run build
# 构建后的文件在 frontend/build/ 目录
# 后端会自动服务这个目录的静态文件
```

## 参数说明

- `-port`: Dashboard HTTP 服务监听端口（默认 8080）
- `-config-addrs`: 配置集群（ShardCtrler）节点地址列表，逗号分隔
- `-shard-groups`: 分片组配置，格式为 `groupId:addr1,addr2,addr3;groupId2:addr4,addr5,addr6`
- `-update-interval`: 状态更新间隔（默认 5s）

## 注意事项

1. **确保集群已启动**：Dashboard 需要连接到运行中的 ShardCtrler 和 ShardKV 节点
2. **端口配置**：确保配置的地址和端口与实际运行的集群一致
3. **网络连通性**：Dashboard 需要能够访问所有配置的节点地址

## 故障排查

### 无法连接到节点
```
Error: failed to collect config status: ...
```
**解决方案**：检查节点地址是否正确，节点是否已启动

### API 返回空数据
**原因**：可能节点还没有完全启动或选举 Leader
**解决方案**：等待几秒后重试，确保集群已经选出 Leader

### 前端无法访问后端 API
**原因**：CORS 或端口配置问题
**解决方案**：
1. 检查后端是否在正确端口运行
2. 前端 `.env` 文件中配置正确的 API 地址
