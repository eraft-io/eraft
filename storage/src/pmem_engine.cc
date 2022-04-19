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

bool PMemEngine::PutK(std::string k, std::string v) {
  auto s = engine_->put(k, v);
  return (s == pmem::kv::status::OK);
}

bool PMemEngine::GetV(std::string k, std::string &v) {
  std::string val;
	auto s = engine_->get(k, &val);
	if (s == pmem::kv::status::NOT_FOUND) {
    return false;
	}
  v = std::move(val);
  return true;
}
}  // namespace storage
