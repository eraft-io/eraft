# EtcdRaftCpp
基于 C++ 实现的 Raft 库 (Raft library implemented by C++)

#### Overview

Raft is a protocol with which a cluster of nodes can maintain a replicated state machine. The state machine is kept in sync through the use of a replicated log. For more details on Raft, see "In Search of an Understandable Consensus Algorithm" (https://raft.github.io/raft.pdf) by Diego Ongaro and John Ousterhout.

#### Basic Dev Schedule

| Feature                       | Development Progress | Review |
| ----------------------------- | -------------------- | ------ |
| Leader election               |                      |        |
| Log replication               |                      |        |
| Log compaction                |                      |        |
| Membership changes            |                      |        |
| Leadership transfer extension |                      |        |

#### Advance Dev Schedule

| Feature                                                      | Development Progress | Review |
| ------------------------------------------------------------ | -------------------- | ------ |
| Optimistic pipelining to reduce log replication latency      |                      |        |
| Flow control for log replication                             |                      |        |
| Batching Raft messages to reduce synchronized network I/O calls |                      |        |
| Batching log entries to reduce disk synchronized I/O         |                      |        |
| Writing to leader's disk in parallel                         |                      |        |
| Internal proposal redirection from followers to leader       |                      |        |
| Automatic stepping down when the leader loses quorum         |                      |        |
| Protection against unbounded log growth when quorum is lost  |                      |        |

#### Build

##### Pre requirement

| Item     | Homepage                                                     |
| -------- | ------------------------------------------------------------ |
| docker   | https://www.docker.com/                                      |
| wget     | https://www.gnu.org/software/wget/                           |
| compiler | Apple clang version 12.0.0 (clang-1200.0.32.29) / Target: x86_64-apple-darwin20.2.0 / Thread model: posix |

##### Build

```
make  // In EtcdRaftCpp project home folder
```

#### Join Us on GoogleGroup

https://groups.google.com/g/etcdraftcppdev

