#include <gtest/gtest.h>

#include "eraftkv.pb.h"
#include "log_entry_cache.h"

TEST(LogEntryCacheTest, Init) {
  LogEntryCache* log_cache = new LogEntryCache();
  ASSERT_EQ(0, log_cache->EntryCount());
  delete log_cache;
}

TEST(LogEntryCacheTest, Append) {
  LogEntryCache*  log_cache = new LogEntryCache();
  eraftkv::Entry* ent_1 = new eraftkv::Entry();
  ent_1->set_id(6);
  ent_1->set_term(1);
  ent_1->set_e_type(eraftkv::EntryType::Normal);
  ent_1->set_data("testv1");
  log_cache->Append(ent_1);
  ASSERT_EQ(1, log_cache->EntryCount());
  ASSERT_EQ(6, log_cache->FirstIndex());
  delete log_cache;
}

TEST(LogEntryCacheTest, GetEntry) {
  LogEntryCache*  log_cache = new LogEntryCache();
  eraftkv::Entry* ent_1 = new eraftkv::Entry();
  ent_1->set_id(6);
  ent_1->set_term(1);
  ent_1->set_e_type(eraftkv::EntryType::Normal);
  ent_1->set_data("testv1");
  log_cache->Append(ent_1);
  eraftkv::Entry* ent_2 = new eraftkv::Entry();
  ent_2->set_id(7);
  ent_2->set_term(1);
  ent_2->set_e_type(eraftkv::EntryType::Normal);
  ent_2->set_data("testv2");
  log_cache->Append(ent_2);
  eraftkv::Entry* ent_3 = new eraftkv::Entry();
  ent_3->set_id(8);
  ent_3->set_term(1);
  ent_3->set_e_type(eraftkv::EntryType::Normal);
  ent_3->set_data("testv3");
  log_cache->Append(ent_3);
  ASSERT_EQ(6, log_cache->FirstIndex());
  auto got_ent = log_cache->Get(7);
  ASSERT_EQ(7, got_ent->id());
  ASSERT_EQ(1, got_ent->term());
  ASSERT_EQ("testv2", got_ent->data());
  ASSERT_EQ(eraftkv::EntryType::Normal, got_ent->e_type());
  delete log_cache;
}

TEST(LogEntryCacheTest, EraseHead) {
  LogEntryCache*  log_cache = new LogEntryCache();
  eraftkv::Entry* ent_1 = new eraftkv::Entry();
  ent_1->set_id(6);
  ent_1->set_term(1);
  ent_1->set_e_type(eraftkv::EntryType::Normal);
  ent_1->set_data("testv1");
  log_cache->Append(ent_1);
  eraftkv::Entry* ent_2 = new eraftkv::Entry();
  ent_2->set_id(7);
  ent_2->set_term(1);
  ent_2->set_e_type(eraftkv::EntryType::Normal);
  ent_2->set_data("testv2");
  log_cache->Append(ent_2);
  eraftkv::Entry* ent_3 = new eraftkv::Entry();
  ent_3->set_id(8);
  ent_3->set_term(1);
  ent_3->set_e_type(eraftkv::EntryType::Normal);
  ent_3->set_data("testv3");
  log_cache->Append(ent_3);
  log_cache->EraseHead(7);
  ASSERT_EQ(7, log_cache->FirstIndex());
  ASSERT_EQ(2, log_cache->EntryCount());
}

TEST(LogEntryCacheTest, EraseTail) {
  LogEntryCache*  log_cache = new LogEntryCache();
  eraftkv::Entry* ent_1 = new eraftkv::Entry();
  ent_1->set_id(6);
  ent_1->set_term(1);
  ent_1->set_e_type(eraftkv::EntryType::Normal);
  ent_1->set_data("testv1");
  log_cache->Append(ent_1);
  eraftkv::Entry* ent_2 = new eraftkv::Entry();
  ent_2->set_id(7);
  ent_2->set_term(1);
  ent_2->set_e_type(eraftkv::EntryType::Normal);
  ent_2->set_data("testv2");
  log_cache->Append(ent_2);
  eraftkv::Entry* ent_3 = new eraftkv::Entry();
  ent_3->set_id(8);
  ent_3->set_term(1);
  ent_3->set_e_type(eraftkv::EntryType::Normal);
  ent_3->set_data("testv3");
  log_cache->Append(ent_3);
  log_cache->EraseTail(7);
  ASSERT_EQ(6, log_cache->FirstIndex());
  ASSERT_EQ(1, log_cache->EntryCount());
  ASSERT_EQ(nullptr, log_cache->Get(7));
  ASSERT_EQ(nullptr, log_cache->Get(8));
  delete log_cache;
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
