# Raft Consensus Implementation

The Raft implementation in eRaft is a standard one but optimized for gRPC and production-like environments.

## Core Features
- **Leader Election**: Handles timeouts, term increments, and split votes.
- **Log Replication**: Ensures that the leader's log is replicated to followers.
- **Safety**: Guarantees that only a node with an up-to-date log can be elected leader.
- **Persistence**: Raft state (term, votedFor, logs) is persisted to disk using a `FilePersister`.
- **Snapshots**: Supports log compaction through snapshots to prevent infinite log growth (integrated with LevelDB).

## Implementation Details
- **Applier Goroutine**: A dedicated goroutine that applies committed entries to the state machine asynchronously to improve throughput.
- **Replicator Goroutines**: One goroutine per peer to handle log replication and heartbeats in parallel.
- **gRPC Integration**: Communication between peers is handled via gRPC, allowing for high concurrency and efficient serialization.

## Key APIs
- `Start(command)`: Proposes a new command to the cluster.
- `GetState()`: Returns the current term and leader status.
- `Snapshot(index, data)`: Truncates the log up to `index` and saves the snapshot.
