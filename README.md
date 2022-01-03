# Bw-Raft

#### Overview

BW-Raft inherits and extends the original ERaft with the following abstractions: (1) secretary nodes that take over expensive log synchronization operations from the leader, relaxing the performance constraint on locks. (2) observer nodes that handle reads only, improving throughput for typical data intensive  services. These abstractions are stateless, allowing elastic scale-out on unreliable yet cheap spot instances.   Our initial code is implemented in golang. You can refer to the code in the Bw-Raft's OriginalCode directory. For better performance, we choose to use cpp to rewrite the code.

One may refer to @inproceedings{10.1145/3326285.3329046, author = {Xu, Zichen and Stewart, Christopher and Huang, Jiacheng}, title = {Elastic, Geo-Distributed RAFT}, year = {2019}, isbn = {9781450367783}, publisher = {Association for Computing Machinery}, address = {New York, NY, USA}, url = { https://doi.org/10.1145/3326285.3329046 } , doi = {10.1145/3326285.3329046},  booktitle = {Proceedings of the International Symposium on Quality of Service}, articleno = {11}, numpages = {9}, location = {Phoenix, Arizona}, series = {IWQoS '19} }

#### Demo

##### Leader election and log replication
![Leader election](document/img/eraft-demo1.gif)

##### Membership changes
![Membership changes](document/img/eraft-demo2.gif)


#### Basic Dev Schedule

| Feature                       | Development Progress | Review |
| ----------------------------- | -------------------- | ------ |
| Leader election               |          done            |    done    |
| Log replication               |          done            |    done    |
| Log compaction                |          done            |    done    |
| Membership changes            |          done            |    done    |
| Leadership transfer extension |          done            |    done    |
| Secretary | Testing              |  |
| Observer | Testing |  |


#### Documents
See the (github wiki)[https://github.com/eraft-io/eraft/wiki] for more detail.

#### Code Style
https://google.github.io/styleguide/cppguide.html

#### Join Us on GoogleGroup

https://groups.google.com/g/eraftio

