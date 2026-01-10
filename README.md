[![Language](https://img.shields.io/badge/Language-Go-blue.svg)](https://golang.org/)
[![License](https://img.shields.io/badge/license-MIT-green)](https://opensource.org/licenses/MIT)

[中文](README_cn.md) | English

### Overview

### Why we need build a distributed system?

Let's first look at the drawbacks of traditional single-node client-server (C/S) or browser-server (B/S) systems:
A single-node architecture means relying on just one machine. The performance of any single machine is inherently limited, and higher-performance machines are significantly more expensive—consider IBM mainframes, for example, which come at a very high cost. Moreover, if this machine fails or the process crashes due to bugs in the code, the system has no fault tolerance and becomes completely unavailable.

After we analyze the shortcomings of single-node systems, we can summarize the design goals of distributed systems

#### 1. Scalability

The distributed system we design must be scalable. The scalability here is that we can obtain higher total system
throughput and better performance by using more machines. Of course, it is not that the more machines, the better the
performance. For some complex computing scenarios, more nodes are not necessarily better performance.

#### 2. Availability

The distributed system will not stop services directly if a machine in the system fails. After a machine fails, the
system can quickly switch traffic to a normal machine and continue to provide services.

#### 3. Consistency

To achieve this, the most important algorithm is the replication algorithm. We need a replication algorithm to ensure
that the data of the dead machine and the machine that is cut to replace it are consistent, usually in the field of
distributed systems. Consistency algorithm to ensure the smooth progress of replication.

### Consistency Algorithm

It is recommended to read [raft small paper](https://raft.github.io/raft.pdf)

Take a look with the following questions:

- what is split brain?

- What is our solution to the split brain?

- why does majority help avoid split brain?

- why the logs?

- why a leader?

- how to ensure at most one leader in a term?

- how does a server learn about a newly elected leader?

- what happens if an election doesn't succeed?

- how does Raft avoid split votes?

- how to choose the election timeout?

- what if old leader isn't aware a new leader is elected?

- how can logs disagree?

- what would we like to happen after a server crashes?

### data sharding

Well, through the basic Raft algorithm, we can achieve a highly available raft server group. We have solved the previous
issues of availability and consistency, but the problem still exists. There is only one leader in a raft server group to
receive read and write traffic. Of course, you can use followers to share part of the read traffic to improve
performance (there will be some issues related to transactions, which we will discuss later). But there is a limit to
what the system can provide.

At this time, we need to think about slicing the requests written by the client, just like map reduce, in the first
stage of map, first cut the huge data set into small ones for processing.

The hash sharding method is used in eraft. We map data into buckets through a hash algorithm, and then different raft
groups are responsible for some of the buckets. How many buckets a raft group can be responsible for can be adjusted.

### System Architecture

#### Concept introduction

##### bucket

It is the logical unit of data management in the cluster, and a grouped service can be responsible for the data of
multiple buckets

##### config table

Cluster configuration table, which mainly maintains the mapping relationship between cluster service groups and buckets.
Before clients access cluster data, they need to go to this table to query the list of service groups where the bucket
is located.

#### service module

##### metaserver

It is mainly responsible for the version management of the cluster configuration table. It internally maintains a
version chain of the cluster configuration table, which can record changes to the cluster configuration.

##### shardkvserver

It is mainly responsible for cluster data storage. Generally, three machines form a raft group to provide
high-availability services to the outside world.

### Build

pre-dependencies

go version >= go 1.21

download code and make it

```
git clone https://github.com/eraft-io/eraft.git

cd eraft
make
```

### run cluster

```
go test -run TestBasicClusterRW tests/integration_test.go -v
```

run basic cluster bench

```
go test -run TestClusterRwBench tests/integration_test.go -v
```

### unit test
generate unit test report

```
make gen-test-coverage 
```

view unit test report

```
make view-test-coverage
```

### Book

Book title: 【Distributed Data Services: Transaction Models, Processing Language, Consistency and Architecture.】

ISBN：978-7-111-73737-7

This book provides a detailed introduction to the implementation principles and code analysis of the eRaft prototype
system in the distributed data services protocol library.

The eraft t project is to industrialize the mit6.824 lab operation into a distributed storage system. We will introduce
the principles of distributed systems in the simplest and straightforward way, and guide you to design and implement an
industrialized distributed storage system.

### Newest document

If you want to check the latest documents, please visit [eraft official website](https://eraft.cn)

### Video tutorials

[bilibili](https://space.bilibili.com/389476201/channel/collectiondetail?sid=481263&spm_id_from=333.788.0.0)

### Ebook for this project

[Shopping link](https://3.cn/1W-jAWMR)
