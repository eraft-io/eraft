## sys_catalog

key encode
```
-------------------------------------------------------------------      
| 'S' | 'Y' | 'S' | '_' | 'C' | 'A' | 'T' | 'A' | 'L' | 'O' | 'G' |    
-------------------------------------------------------------------    
```
value encode
```
--------------------------------
|  SysCatalog protobuf message |
--------------------------------
```

- SysCatalog message
```
{
    uint64 vecset_count
    uint64 max_vecset_id
    uint64 mem_used
    uint64 disk_used
    map<string, uint64> vecset_name2id
}
```

## vecset

```
key encode

---------------------------------------------------      
| 'V' | 'E' | 'C' | 'S' | 'E' | 'T' |  ID(uint64) |       
---------------------------------------------------      

value encode

----------------------------
|  Vecset protobuf message |
----------------------------

```

- Vecset protobuf message 

```
{
    uint64_t    id
    bytes       name
    uint64_t    vec_count
    uint64_t    max_vec_id
    uint64_t    used_disk_capacity
    uint64_t    used_mem_capacity
    uint64_t    c_time
    uint64_t    m_time
}
```

## vec

```

key encode

----------------------------------------------------------------------------      
| 'V' | 'E' | 'C' | 'T' | 'O' | 'R' |  VECSET_ID(uint64) |  VEC_ID(uint64) | 
----------------------------------------------------------------------------      

value encode
------------------------
| Vec protobuf message |
------------------------

```

- Vec protobuf message

```
{
    uint64_t    id
    uint64_t    dim
    repeated double vdata
    bytes       vlabel
    uint64_t    c_time
    uint64_t    m_time
}
```
