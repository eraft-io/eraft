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
 * @file log_entry_cache_benchmark.cc
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-05-21
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <benchmark/benchmark.h>

#include "eraftkv.pb.h"
#include "log_entry_cache.h"

/**
 * @brief
 *
 * @param state
 */
static void BM_LogCacheAppend(benchmark::State& state) {
  LogEntryCache*  log_cache = new LogEntryCache();
  eraftkv::Entry* ent = new eraftkv::Entry();
  ent->set_id(6);
  ent->set_term(1);
  ent->set_e_type(eraftkv::EntryType::Normal);
  ent->set_data(
      "48b29fd8d7828b3a3a71e1c9eb2661a140bb3e185552ef2d5c0292f816ed0e36");

  for (auto _ : state) {
    log_cache->Append(ent);
  }

  delete log_cache;
}

BENCHMARK(BM_LogCacheAppend);

BENCHMARK_MAIN();
