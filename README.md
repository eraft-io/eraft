### for dev test

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