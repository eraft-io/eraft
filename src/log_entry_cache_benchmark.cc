#include <benchmark/benchmark.h>
#include "log_entry_cache.h"
#include "eraftkv.pb.h"

static void BM_LogCacheAppend(benchmark::State& state) {
    LogEntryCache* log_cache = new LogEntryCache();
    eraftkv::Entry* ent = new eraftkv::Entry(); 
    ent->set_id(6);
    ent->set_term(1);
    ent->set_e_type(eraftkv::EntryType::Normal);
    ent->set_data("48b29fd8d7828b3a3a71e1c9eb2661a140bb3e185552ef2d5c0292f816ed0e36");

    for (auto _ : state) {
        log_cache->Append(ent);
    }

    delete log_cache;
}

BENCHMARK(BM_LogCacheAppend);

BENCHMARK_MAIN();
