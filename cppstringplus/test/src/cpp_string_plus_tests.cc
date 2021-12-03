
#include <cppstringplus/cppstringplus.h>
#include <gtest/gtest.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include "rocksdb/db.h"

// #include <assert>
#include <limits>
#include <string>
#include <vector>

TEST(CppStringPlusTests, ToLower) {
  rocksdb::DB* db;
  rocksdb::Options options;
  options.create_if_missing = true;
  rocksdb::Status status = rocksdb::DB::Open(options, "/tmp/testdb", &db);
  assert(status.ok());
}