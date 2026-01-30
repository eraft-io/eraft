# Benchmark & Performance

The `eRaft` system includes built-in benchmarking tools to measure the performance of your cluster.

## Benchmarking Tool
The `shardkvclient` includes a `bench` command designed to test the write throughput of the system.

### Usage
```bash
./output/shardkvclient -ctrlers "localhost:50051,localhost:50052,localhost:50053" bench <num_requests>
```

### What it does:
1. **Random Keys**: Generates keys with random first letters ('a'-'z') to ensure requests are distributed across all shards and groups.
2. **Sequential Load**: Sends a stream of `Put` requests to the cluster.
3. **Metrics**: Reports the total time taken and the average throughput in requests per second (req/s).

## Observed Performance
Typical performance on a local machine with 3 nodes per group:
- **Throughput**: ~150 - 300 req/s (depending on disk I/O and consensus latency).
- **Consistency**: 100% linearizable consistency guaranteed by Raft.

## Optimization Tips
- **Disk Speed**: Since LevelDB and Raft persistence are synchronous, using an SSD will significantly improve performance.
- **Batching**: Future versions may include log batching to further increase throughput.
