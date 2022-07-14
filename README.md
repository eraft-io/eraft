[![Language](https://img.shields.io/badge/Language-Go-blue.svg)](https://golang.org/)
![License](https://img.shields.io/badge/license-Apache-blue.svg)

## WellWood

Wellwood is a distributed block storage system, based on multi raft algorithm, we store file blocks in a distributed manner. Ensure high availability and provide ultra large capacity storage support.

The following is our architecture diagram:

![WellWood](https://cdn.nlark.com/yuque/0/2022/png/29306112/1656687604705-cefdbe9e-3242-4173-871f-fdb11fcacd83.png)

## quick start

run meta server
```
./meta_server -id 0
./meta_server -id 1
./meta_server -id 2
```

add server group to cluster

```
./wellwood-ctl add_server_group 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 1 127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090
```

run block_server
```
./block_server -id  0 -gid 1
./block_server -id  1 -gid 1
./block_server -id  2 -gid 1
```

test write

run testcase in test/clientsdk/upload_file_test.go

put object to cluster

```
./wellwood-ctl put_object 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 /Users/colin/Downloads/DSC_7627.JPG 3f8602ef-9488-419f-a485-4df0cbd73c3  
```
this operation will return object id

get objects list from cluster

```
./wellwood-ctl get_objects 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090
```

get object from cluster
```
./wellwood-ctl get_object 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 3f8602ef-9488-419f-a485-4df0cbd73c3 2b74a567-cde0-475b-8802-c663581af9c1 /Users/colin/Desktop/test.JPG
```

