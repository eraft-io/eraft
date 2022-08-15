### 在 k8s 里面运行 wellwood

#### 启动 minikube 
minikube start --image-mirror-country='cn'

#### 在 k8s 创建一个 wellwood 集群需要的资源

```
kubectl apply -f wellwood-metaserver-storage-class.yaml
kubectl apply -f wellwood-metaserver-statefulset.yaml
kubectl apply -f wellwood-metaserver-services.yaml

kubectl apply -f wellwood-blockserver-storage-class.yaml
kubectl apply -f wellwood-blockserver-statefulset.yaml
kubectl apply -f wellwood-blockserver-services.yaml
```

#### 添加 block server 分组到集群中 

kubectl run wellwood-client --image=eraft/eraft_wellwood:v3 -i -t --rm --restart=Never -- wellwood-ctl add_server_group wellwood-metaserver-0.wellwood-metaserver:8088,wellwood-metaserver-1.wellwood-metaserver:8089,wellwood-metaserver-2.wellwood-metaserver:8090 1 wellwood-blockserver-0.wellwood-blockserver:7088,wellwood-blockserver-1.wellwood-blockserver:7089,wellwood-blockserver-2.wellwood-blockserver:7090

####  获取集群拓扑信息
kubectl run wellwood-client --image=eraft/eraft_wellwood:v3 -i -t --rm --restart=Never -- wellwood-ctl get_cluster_topo wellwood-metaserver-0.wellwood-metaserver:8088,wellwood-metaserver-1.wellwood-metaserver:8089,wellwood-metaserver-2.wellwood-metaserver:8090

- 输出
```
{
	"server_group_metas": {
		"config_version": 1,
		"slots": [1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
		"server_groups": {
			"1": "wellwood-blockserver-0.wellwood-blockserver:7088,wellwood-blockserver-1.wellwood-blockserver:7089,wellwood-blockserver-2.wellwood-blockserver:7090"
		}
	}
}
```