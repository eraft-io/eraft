## k8s 中运行 eraft 的方法

### 安装 minikube

```
minikube start --image-mirror-country='cn'
```

### 部署 eraft services

```
kubectl apply -f eraft-services.yaml
```

### 部署 StorageClass 动态存储卷

```
kubectl apply -f eraft-storage-class.yaml
```

### 部署 kv_server 分组服务的 StatefulSet

```
kubectl apply -f eraft-statefulset.yaml
```
