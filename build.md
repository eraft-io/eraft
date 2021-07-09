# modify rocksdb main CMakeLists.txt with options

```
CMAKE_DEPENDENT_OPTION(WITH_TESTS "build with tests" OFF
  "CMAKE_BUILD_TYPE STREQUAL Debug" OFF)
option(WITH_BENCHMARK_TOOLS "build with benchmarks" OFF)
option(WITH_CORE_TOOLS "build with ldb and sst_dump" OFF)
option(WITH_TOOLS "build with tools" OFF)

```