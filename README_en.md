[![Language](https://img.shields.io/badge/Language-Go-blue.svg)](https://golang.org/)
[![License](https://img.shields.io/badge/license-MIT-green)](https://opensource.org/licenses/MIT)

[中文](README.md) | English
### Overview

The eraft t project is to industrialize the mit6.824 lab operation into a distributed storage system. We will introduce the principles of distributed systems in the simplest and straightforward way, and guide you to design and implement an industrialized distributed storage system.

### Newest document

If you want to check the latest documents, please visit [eraft official website](https://eraft.cn)

### Why we need build a distributed system?

First, let's look at the shortcomings of traditional single-node C/S or B/S systems:

A single node means that only one machine is used, the performance of the machine is limited, and the machine with better performance is more expensive, like IBM's mainframe, the price is very expensive. At the same time, if the machine hangs up or the process is abnormal due to a bug in the code written, it cannot tolerate faults, and the system is directly unavailable.

After we analyze the shortcomings of single-node systems, we can summarize the design goals of distributed systems

#### 1. Scalability
The distributed system we design must be scalable. The scalability here is that we can obtain higher total system throughput and better performance by using more machines. Of course, it is not that the more machines, the better the performance. For some complex computing scenarios, more nodes are not necessarily better performance.

#### 2. Availability
The distributed system will not stop services directly if a machine in the system fails. After a machine fails, the system can quickly switch traffic to a normal machine and continue to provide services.

#### 3. Consistency
To achieve this, the most important algorithm is the replication algorithm. We need a replication algorithm to ensure that the data of the dead machine and the machine that is cut to replace it are consistent, usually in the field of distributed systems. Consistency algorithm to ensure the smooth progress of replication.

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

Well, through the basic Raft algorithm, we can achieve a highly available raft server group. We have solved the previous issues of availability and consistency, but the problem still exists. There is only one leader in a raft server group to receive read and write traffic. Of course, you can use followers to share part of the read traffic to improve performance (there will be some issues related to transactions, which we will discuss later). But there is a limit to what the system can provide.

At this time, we need to think about slicing the requests written by the client, just like map reduce, in the first stage of map, first cut the huge data set into small ones for processing.

The hash sharding method is used in eraft. We map data into buckets through a hash algorithm, and then different raft groups are responsible for some of the buckets. How many buckets a raft group can be responsible for can be adjusted.

### System Architecture

![System Architecture](docs/imgs/eraftdb_arch.png)

#### Concept introduction

##### bucket

It is the logical unit of data management in the cluster, and a grouped service can be responsible for the data of multiple buckets

##### config table

Cluster configuration table, which mainly maintains the mapping relationship between cluster service groups and buckets. Before clients access cluster data, they need to go to this table to query the list of service groups where the bucket is located.

#### service module

##### configserver

It is mainly responsible for the version management of the cluster configuration table. It internally maintains a version chain of the cluster configuration table, which can record changes to the cluster configuration.

##### shardserver

It is mainly responsible for cluster data storage. Generally, three machines form a raft group to provide high-availability services to the outside world.


### Build

pre-dependencies

go version >= go1.17.6

download code and make it

```
git clone https://github.com/eraft-io/eraft.git

cd eraft
make
```

The output bin file is in the output directory

Get started with eraft quickly and try distributed systems

1.Start a set of configuration services

```
# 8088 
./cfgserver 0 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090

# 8089
./cfgserver 1 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090

# 8090
./cfgserver 2 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090
```

2.Add an initial cluster grouping

```
./cfgcli 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 join 1 127.0.0.1:6088,127.0.0.1:6089,127.0.0.1:6090
```

3.Boot the cluster group

```
./shardserver 0 1 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 127.0.0.1:6088,127.0.0.1:6089,127.0.0.1:6090

./shardserver 1 1 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 127.0.0.1:6088,127.0.0.1:6089,127.0.0.1:6090

./shardserver 2 1 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 127.0.0.1:6088,127.0.0.1:6089,127.0.0.1:6090
```

4.Add and start a group again

```
./cfgcli 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 join 2 127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090

./shardserver 0 2 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090

./shardserver 1 2 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090

./shardserver 2 2 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 127.0.0.1:7088,127.0.0.1:7089,127.0.0.1:7090
```

5.Query group status

```
./cfgcli 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 query
```

6.Set traffic to shard

```
./cfgcli 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 move 0-4 1

./cfgcli 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 move 5-9 2
```

7.read and write data

```
./shardcli 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 put testkey testvalue
./shardcli 127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090 get testkey
```
