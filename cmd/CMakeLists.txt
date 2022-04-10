set(cmd)

list(APPEND cmd pmem_redis.cc)

set(cmd_INCLUDE_DIR ${cmd_SOURCE_DIR})

add_library(cmd ${cmd})
target_include_directories(cmd PUBLIC ${cmd_INCLUDE_DIR})


add_executable(pmemkv_redisd pmem_redis.cc)
target_include_directories(pmemkv_redisd PUBLIC ${cmd_INCLUDE_DIR})
target_link_libraries(pmemkv_redisd pthread pmemkv cmd)
