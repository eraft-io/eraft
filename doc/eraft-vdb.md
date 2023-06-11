# Introduction
eraft-vdb is vector database, it's storage layer is implemented based on eraftkv, supports very large vector data storage and fast similar high-dimensional vector search.

## High-Level Overview of eraft-vdb's Architecture

![eraft-vdb](eraft-vdb.png)

eraft-vdb consists of the following modules:

### Vecset

Vecset is a collection of Vec, You can create a vecset through restful api, and then add vector to it.

### Vec

Vec stores a single vector, include vector id, vector data and additional comments:

- vid:
A unique identifier of your vector

- vdata:


- vextra:

### Storage Cluster

### RestfulAPI
