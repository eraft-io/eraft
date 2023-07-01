### Meta key design

K -> V

[ ['S'] ['G'] ['M'] ['E'] ['T'] ['A'] [ID (int64)]] ->  ShardGroup protobuf serialized message

- Join

example:

[ ['S'] ['G'] ['M'] ['E'] ['T'] ['A'] [1] ] -> 
{ id: 1, slots: [], servers: ['0-127.0.0.1:8088-UP,1-127.0.0.1:8089-UP,2-127.0.0.1:8090-UP'], leader_id: 0}

- Leave


- Query
