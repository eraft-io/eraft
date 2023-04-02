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

TEST(RockDBStorageImplTest, PutPut) {
    RocksDBStorageImpl* kv_store = new RocksDBStorageImpl("/tmp/testdb");
    ASSERT_EQ(kv_store->PutKV(std::string("testkey"), std::string("testval")), EStatus::kOk);
    delete kv_store;
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
