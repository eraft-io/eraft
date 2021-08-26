# eraft
Raft library implemented by C++, inspired by etcd golang version and pingcap tinykv

ERaft supports academic research projects on scaling out Raft to spot instances. One of our main work has been published in IWQoS 2019.

One may refer to @inproceedings{10.1145/3326285.3329046, author = {Xu, Zichen and Stewart, Christopher and Huang, Jiacheng}, title = {Elastic, Geo-Distributed RAFT}, year = {2019}, isbn = {9781450367783}, publisher = {Association for Computing Machinery}, address = {New York, NY, USA}, url = {https://doi.org/10.1145/3326285.3329046}, doi = {10.1145/3326285.3329046},  booktitle = {Proceedings of the International Symposium on Quality of Service}, articleno = {11}, numpages = {9}, location = {Phoenix, Arizona}, series = {IWQoS '19} }

Another team of the same root is now working on a stable version called BW-Raft, which inherits the implementation from ERaft. One may refer the latest note from the Good Lab, ![good lab](good.ncu.edu.cn)

#### Overview

Raft is a protocol with which a cluster of nodes can maintain a replicated state machine. The state machine is kept in sync through the use of a replicated log. For more details on Raft, see "In Search of an Understandable Consensus Algorithm" (https://raft.github.io/raft.pdf) by Diego Ongaro and John Ousterhout.

#### Demo

##### Leader election and log replication
![Leader election](Doc/img/eraft-demo1.gif)

##### Membership changes
![Membership changes](Doc/img/eraft-demo2.gif)


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

