# eraft
Raft library implemented by C++, inspired by etcd golang version

#### Overview

Raft is a protocol with which a cluster of nodes can maintain a replicated state machine. The state machine is kept in sync through the use of a replicated log. For more details on Raft, see "In Search of an Understandable Consensus Algorithm" (https://raft.github.io/raft.pdf) by Diego Ongaro and John Ousterhout.

#### Demo

![](Doc/img/eraft-demo.gif)

#### Basic Dev Schedule

| Feature                       | Development Progress | Review |
| ----------------------------- | -------------------- | ------ |
| Leader election               |          done            |    done    |
| Log replication               |          done            |    done    |
| Log compaction                |          done            |    done    |
| Membership changes            |          done            |    done    |
| Leadership transfer extension |          done            |    done    |


#### Documents
See the (github wiki)[https://github.com/eraft-io/eraft/wiki] for more detail.

#### Code Style
https://google.github.io/styleguide/cppguide.html

#### Join Us on GoogleGroup

https://groups.google.com/g/eraftio

