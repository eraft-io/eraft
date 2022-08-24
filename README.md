[![Language](https://img.shields.io/badge/Language-Go-blue.svg)](https://golang.org/)
![License](https://img.shields.io/badge/license-Apache-blue.svg)

## WellWood

Wellwood is a distributed block storage system, based on multi raft algorithm, we store file blocks in a distributed manner. Ensure high availability and provide ultra large capacity storage support.

The following is our architecture diagram:

![WellWood](https://cdn.nlark.com/yuque/0/2022/png/29306112/1656687604705-cefdbe9e-3242-4173-871f-fdb11fcacd83.png)

## build 
```
git clone https://github.com/eraft-io/eraft.git
cd eraft
make
```

## quick start

run meta server
```
./meta_server -id 0 -peers 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090
./meta_server -id 1 -peers 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090
./meta_server -id 2 -peers 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090
```

add server group to cluster

```
./wellwood-ctl add_server_group 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 1 127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090
```

check cluster topo
```
./wellwood-ctl  get_cluster_topo 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090
```

output
```
{
	"server_group_metas": {
		"config_version": 1,
		"slots": [1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
		"server_groups": {
			"1": "127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090"
		}
	}
}
```

run block_server
```
./block_server -id  0 -gid 1 -peers 127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090
./block_server -id  1 -gid 1 -peers 127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090
./block_server -id  2 -gid 1 -peers 127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090
```

run web dashboard
```
./dashboard -meta_addrs 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090
```

to http://127.0.0.1:12008/ 

![WellWood Dashboard](https://cdn.nlark.com/yuque/0/2022/png/29306112/1660571703459-a2775f27-0436-4e3b-a2ee-f81e912a794c.png)

## Next step

- Full [documentation](https://eraft.cn/wellwood/)
