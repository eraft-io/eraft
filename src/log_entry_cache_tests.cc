// MIT License

// Copyright (c) 2023 ERaftGroup

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

/**
 * @file log_entry_cache_tests.cc
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-05-21
 *
 * @copyright Copyright (c) 2023
 *
 */
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
