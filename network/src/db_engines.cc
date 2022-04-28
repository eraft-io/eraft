// MIT License

// Copyright (c) 2022 eraft dev group

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

#include <network/db_engines.h>
#include <storage/pmem_engine.h>

namespace network {

DBEngines::DBEngines(std::string kvPath, std::string raftPath)
    : kvPath_(kvPath), raftPath_(raftPath) {
  const uint64_t PMEM_USED_SIZE_DEFAULT = 1024UL * 1024UL * 1024UL;
  kvDB_ = std::make_shared<storage::PMemEngine>(kvPath, "radix",
                                                PMEM_USED_SIZE_DEFAULT);
  raftDB_ = std::make_shared<storage::PMemEngine>(raftPath, "radix",
                                                  PMEM_USED_SIZE_DEFAULT);
}

DBEngines::~DBEngines() {}

bool DBEngines::WriteKV(storage::WriteBatch &batch) {
  kvDB_->PutWriteBatch(batch);
}

bool DBEngines::WriteRaft(storage::WriteBatch &batch) {
  raftDB_->PutWriteBatch(batch);
}

bool DBEngines::Close() {}

bool DBEngines::Destory() {}

}  // namespace network
