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

#ifndef ERAFT_PMEM_ENGINE_H_
#define ERAFT_PMEM_ENGINE_H_

#include <storage/engine_interface.h>
#include <storage/write_batch.h>

#include <cstdint>
#include <libpmemkv.hpp>
#include <memory>
#include <string>

namespace storage {

class PMemEngine : public StorageEngineInterface {
 public:
  // Constructor for pmem engine
  PMemEngine(std::string dbPath, std::string engName, uint64_t dbSize,
             bool createIfMissing = true);

  ~PMemEngine();

  EngOpStatus PutK(std::string k, std::string v) override;

  EngOpStatus GetV(std::string k, std::string& v) override;

  EngOpStatus PutWriteBatch(WriteBatch& batch) override;

  EngOpStatus RemoveK(std::string k) override;

  EngOpStatus RangeQuery(std::string startK, std::string endK,
                         std::vector<std::string>& matchKeys,
                         std::vector<std::string>& matchValues);

 private:
  std::unique_ptr<pmem::kv::db> engine_;
};

}  // namespace storage

#endif