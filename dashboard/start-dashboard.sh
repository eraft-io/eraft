#!/bin/bash

# eRaft Dashboard 快速启动脚本

echo "=== eRaft Dashboard 快速启动 ==="
echo ""

# 检查是否已构建
if [ ! -f "../output/dashboard-server" ]; then
    echo "Dashboard 服务器未构建，正在构建..."
    cd ..
    make builddashboard
    if [ $? -ne 0 ]; then
        echo "构建失败！请检查错误信息。"
        exit 1
    fi
    cd dashboard
fi

echo "启动 Dashboard 服务器..."
echo ""
echo "配置信息："
echo "  - 监听端口: 8080"
echo "  - 配置集群: localhost:8001, localhost:8002, localhost:8003"
echo "  - 分片组 100: localhost:9001, localhost:9002, localhost:9003"
echo "  - 分片组 101: localhost:9004, localhost:9005, localhost:9006"
echo "  - 更新间隔: 5s"
echo ""
echo "访问地址: http://localhost:8080/api/dashboard"
echo "前端界面: 需要先构建前端（见 README.md）"
echo ""

../output/dashboard-server \
  -port=8080 \
  -config-addrs="localhost:8001,localhost:8002,localhost:8003" \
  -shard-groups="100:localhost:9001,localhost:9002,localhost:9003;101:localhost:9004,localhost:9005,localhost:9006" \
  -update-interval=5s
