# ERaftKV 

ERaftKV is a persistent distributed KV storage system, uses the Raft protocol to ensure data consistency, At the same time, it supports sharding to form multi shard large-scale data storage clusters.

# Features
- Strong and consistent data storage ensures secure and reliable data persistence in distributed systems.
- Support KV data type operations, including PUT, GET, DEL, and SCAN operations on keys. When users operate on cluster data, they must ensure the persistence of the operation and the sequential consistency of reading and writing.
- Dynamically configure the cluster, including adding and deleting nodes, adding and deleting cluster sharding configurations, including which keyrange the cluster sharding is responsible for.
- Support for snapshot taking with the raft to compress and merge logs. During the snapshot, it is required to not block data read and write.
- Support switching to a specifying leader.
- Raft elections support PreVote, and newly added nodes do not participate in the election by tracking data to avoid triggering unnecessary elections.
- Raft read optimization: adding a read queue ensures that the leader node returns a read request after submitting it, and unnecessary logs are not written.
- Support data migration, multi-shard scale out.

# Getting Started

## Building and run test using [GitHub Actions](https://github.com/features/actions)

All you need to do is submit a Pull Request to our repository, and all compile, build, and testing will be automatically executed.

You can check the [ERatfKVGitHubActions](https://github.com/eraft-io/eraft/actions) See the execution process of code build tests.

## Building on your local machine.

To compile eraftkv, you will need:
- Docker

To build:
```
make gen-protocol-code
make build-dev
```

Run test:
```
make tests
```

If you want to build image youtself
```
make image
```

# Documentation

## Run Demo

- set up demo cluster

```
./build/eraftkv 0 /tmp/kv_db0 /tmp/log_db0 127.0.0.1:8088
./build/eraftkv 1 /tmp/kv_db1 /tmp/log_db1 127.0.0.1:8089
./build/eraftkv 2 /tmp/kv_db2 /tmp/log_db2 127.0.0.1:8090

```

- put kv
```
TEST(ERaftKvServerTest, ClientOperationReq) {
    auto                           chan_ = grpc::CreateChannel("127.0.0.1:8088",
                                     grpc::InsecureChannelCredentials());
    std::unique_ptr<ERaftKv::Stub> stub_(ERaftKv::NewStub(chan_));
    ClientContext                  context;
    eraftkv::ClientOperationReq        req;
    time_t time_in_sec;
    time(&time_in_sec);
    req.set_op_timestamp(static_cast<uint64_t>(time_in_sec));
    auto kv_pair = req.add_kvs();
    kv_pair->set_key("testkey");
    kv_pair->set_value("testval");
    kv_pair->set_op_type(eraftkv::ClientOpType::Put);
    eraftkv::ClientOperationResp       resp;
    auto status = stub_->ProcessRWOperation(&context, req, &resp);
}
```

# Contributing

You can quickly participate in development by following the instructions in [CONTROLUTING.md](https://github.com/eraft-io/eraft/blob/master/CONTRIBUTING.md)
