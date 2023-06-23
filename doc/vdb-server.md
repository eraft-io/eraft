## vdb-server RESP protocol for vector design

### 1.Create a Vecset

- Syntax
```
VECSET ADD vecset_name vecset_size 
```
- Return
vecset id

### 2.Get a Vecset

- Syntax
```
VECSET GET vecset_name
```
- Return

Bulk string reply:

```
vecset_size
vec_count
vecdata_disk_size
vecdata_mem_size
c_time
```

### 3.Add Vecs to Vecset

- Syntax
```
VEC ADD vecset_name vector_data vector_label  [vector_data vector_label ...]
```

- Return
The status of the add operation.

### 4.Search from a vecset

-Syntax
```
VEC SEARCH vecset_name vector_data DESC count
```
- Return 

search result vector data similar with 
vector_data.

```
vec_id score vec_label [vec_id score vec_label ...]
```
