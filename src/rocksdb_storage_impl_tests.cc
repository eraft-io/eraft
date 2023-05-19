/**
 * @file rocksdb_storage_impl_tests.cc
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-04-02
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <gtest/gtest.h>

#include "rocksdb_storage_impl.h"
#include "util.h"

TEST(RockDBStorageImplTest, PutGet) {
  std::string         testk = "testkey";
  std::string         testv = "testval";
  std::string         not_exist_key = "not_exist";
  RocksDBStorageImpl* kv_store = new RocksDBStorageImpl("/tmp/testdb");
  ASSERT_EQ(kv_store->PutKV(testk, testv), EStatus::kOk);
  ASSERT_EQ(kv_store->GetKV(testk), testv);
  ASSERT_EQ(kv_store->GetKV(""), std::string(""));
  delete kv_store;
  DirectoryTool::DeleteDir("/tmp/testdb");
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
