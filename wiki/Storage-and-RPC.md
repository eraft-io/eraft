# Storage & RPC Layer

eRaft moves away from memory-based storage and internal RPC simulators to industry-standard components.

## Storage: LevelDB
We use **LevelDB** as the underlying state machine storage. 
- **Persistence**: Unlike in-memory maps, data survives node restarts.
- **Efficiency**: LevelDB provides high-performance read/write operations with SSTables and LSM-trees.
- **Isolation**: Each Raft node has its own LevelDB directory (e.g., `data/skv100_0`).

## RPC: gRPC & Protobuf
All communication in eRaft is built on **gRPC**.
- **Protobuf**: Structured message definitions in `.proto` files ensure type safety and efficient binary serialization.
- **Concurrency**: gRPC's HTTP/2 based multiplexing allows for efficient parallel RPCs.
- **Dual Compatibility**: Our implementation maintains a thin wrapper to remain compatible with the original MIT 6.824 test framework while running natively on gRPC for production use.

## Path Management
Log entries and data are stored in the `data/` directory, which is ignored by Git to keep the repository clean.
