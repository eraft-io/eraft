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

#include <storage/pmem_engine.h>

#include <iostream>
#include <memory>

namespace storage {

// Constructor for pmem engine
PMemEngine::PMemEngine(std::string dbPath, std::string engName, uint64_t dbSize,
                       bool createIfMissing) {
  pmem::kv::config cfg;
  pmem::kv::status s = cfg.put_path(dbPath);
  if (s != pmem::kv::status::OK) {
    std::cerr << "put path to pmem config err " << std::endl;
    exit(-1);
  }
  cfg.put_size(dbSize);
  cfg.put_create_if_missing(createIfMissing);
  std::unique_ptr<pmem::kv::db> newEng(new pmem::kv::db());
  engine_ = std::move(newEng);
  s = engine_->open(engName, std::move(cfg));
  if (s != pmem::kv::status::OK) {
    std::cerr << "open pmemkv error" << std::endl;
    exit(-1);
  }
}

PMemEngine::~PMemEngine() {}

EngOpStatus PMemEngine::PutK(std::string k, std::string v) {
  auto s = engine_->put(k, v);
  if (s == pmem::kv::status::OK) {
    return EngOpStatus::OK;
  }
  return EngOpStatus::ERROR;
}

EngOpStatus PMemEngine::GetV(std::string k, std::string& v) {
  std::string val;
  auto s = engine_->get(k, &val);
  if (s == pmem::kv::status::NOT_FOUND) {
    return EngOpStatus::NOT_FOUND;
  }
  v = std::move(val);
  return EngOpStatus::OK;
}

EngOpStatus PMemEngine::RemoveK(std::string k) {
  auto s = engine_->remove(k);

  return (s == pmem::kv::status::OK);
}

EngOpStatus PMemEngine::PutWriteBatch(WriteBatch& batch) {
  for (auto& item : batch.GetItems()) {
    switch (item.first) {
      case BacthOpCode::Put: {
        PutK(item.second.first, item.second.second);
        break;
      }
      case BacthOpCode::Delete: {
        RemoveK(item.second.first);
        break;
      }
      default:
        break;
    }
  }
}

EngOpStatus PMemEngine::RangeQuery(std::string startK, std::string endK,
                                   std::vector<string>& matchValues) {
  auto rangeIter = engine_->new_read_iterator();
  auto& sIt = rangeIter.get_value();
  auto seekStatus = sIt.seek_higher_eq(startK);
  do {
    pmem::kv::result<pmem::kv::string_view> keyRes = sIt.key();
    std::string currentKey = keyRes.get_value().data();
    if (currentKey.compare(endK) <= 0) {
      std::string currentValue = sIt.read_range().get_value().data();
      matchValues.push_back(currentValue);
    } else {
      return EngOpStatus::OK;
    }
  } while (sIt.next() == pmem::kv::status::OK) return EngOpStatus::OK;
}
}  // namespace storage
