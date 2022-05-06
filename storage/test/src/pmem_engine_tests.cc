#include <gtest/gtest.h>
#include <storage/engine_interface.h>
#include <storage/pmem_engine.h>

#include <iostream>

namespace storage {

TEST(PMemEngineTests, PMemEngine) {
  const uint64_t PMEM_USED_SIZE_DEFAULT = 1024UL * 1024UL * 1024UL;
  std::shared_ptr<StorageEngineInterface> engFace =
      std::make_shared<PMemEngine>("/tmp/test_db", "radix",
                                   PMEM_USED_SIZE_DEFAULT);
}

TEST(PMemEngineTests, PMemEngineTestPutGet) {
  const uint64_t PMEM_USED_SIZE_DEFAULT = 1024UL * 1024UL * 1024UL;
  std::shared_ptr<StorageEngineInterface> engFace =
      std::make_shared<PMemEngine>("/tmp/test_db_put_get", "radix",
                                   PMEM_USED_SIZE_DEFAULT);
  ASSERT_EQ(EngOpStatus::OK, engFace->PutK("testkey", "hello eraft!"));
  std::string gotV;
  ASSERT_EQ(EngOpStatus::OK, engFace->GetV("testkey", gotV));
  ASSERT_EQ("hello eraft!", gotV);
}

TEST(PMemEngineTests, PMemEngineTestGetFromExistsDB) {
  const uint64_t PMEM_USED_SIZE_DEFAULT = 1024UL * 1024UL * 1024UL;
  std::shared_ptr<StorageEngineInterface> engFace =
      std::make_shared<PMemEngine>("/tmp/test_db_put_get", "radix",
                                   PMEM_USED_SIZE_DEFAULT);
  std::string gotV;
  ASSERT_EQ(EngOpStatus::OK, engFace->GetV("testkey", gotV));
  ASSERT_EQ("hello eraft!", gotV);
}

TEST(PMemEngineTests, PMemEngineTestsPutWriteBatch) {
  const uint64_t PMEM_USED_SIZE_DEFAULT = 1024UL * 1024UL * 1024UL;
  std::shared_ptr<StorageEngineInterface> engFace =
      std::make_shared<PMemEngine>("/tmp/test_db_put_batch", "radix",
                                   PMEM_USED_SIZE_DEFAULT);
  WriteBatch testBatch;
  testBatch.Put("test1", "v1");
  testBatch.Put("test2", "v2");
  engFace->PutWriteBatch(testBatch);
  std::string gotV1, gotV2;
  ASSERT_EQ(EngOpStatus::OK, engFace->GetV("test1", gotV1));
  ASSERT_EQ(EngOpStatus::OK, engFace->GetV("test2", gotV2));
  ASSERT_EQ("v1", gotV1);
  ASSERT_EQ("v2", gotV2);

  WriteBatch delBatch;
  delBatch.Delete("test1");
  delBatch.Delete("test2");
  engFace->PutWriteBatch(delBatch);
  ASSERT_EQ(EngOpStatus::NOT_FOUND, engFace->GetV("test1", gotV1));
  ASSERT_EQ(EngOpStatus::NOT_FOUND, engFace->GetV("test2", gotV2));
}

TEST(PMemEngineTests, PMemEngineTestsRangeQuery) {
  const uint64_t PMEM_USED_SIZE_DEFAULT = 1024UL * 1024UL * 1024UL;
  std::shared_ptr<StorageEngineInterface> engFace =
      std::make_shared<PMemEngine>("/tmp/test_db_range_query", "radix",
                                   PMEM_USED_SIZE_DEFAULT);
  WriteBatch testBatch;
  testBatch.Put("test1", "v1");
  testBatch.Put("test2", "v2");
  testBatch.Put("test3", "v3");
  engFace->PutWriteBatch(testBatch);
  std::vector<std::string> matchKeys, matchValues;
  engFace->RangeQuery("test13", "test23", matchKeys, matchValues);
  for (auto& key : matchKeys) {
    std::cout << key << std::endl;
  }
  matchKeys.clear();
  matchValues.clear();
  engFace->RangeQuery("test2", "test4", matchKeys, matchValues);
  for (auto& key : matchKeys) {
    std::cout << key << std::endl;
  }
}

}  // namespace storage
