set(raftcore_sources)

list(APPEND raftcore_sources entry.cc)
list(APPEND raftcore_sources raft.cc)

set(raftcore_INCLUDE_DIR ${raftcore_sources_SOURCE_DIR})

add_library(raftcore ${raftcore_sources})
target_link_libraries(raftcore PUBLIC
    network
)
target_include_directories(raftcore PUBLIC ${raftcore_INCLUDE_DIR})
