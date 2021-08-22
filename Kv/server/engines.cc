// MIT License

// Copyright (c) 2021 eraft dev group

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

#include <Kv/engines.h>
#include <Kv/file.h>

namespace kvserver {

//
// init engines, contains kvDB_ and raftDB_;
//
Engines::Engines(std::string kvPath, std::string raftPath)
    : kvPath_(kvPath), raftPath_(raftPath) {
  rocksdb::Options opts;
  opts.create_if_missing = true;
  rocksdb::Status s1 = rocksdb::DB::Open(opts, kvPath, &kvDB_);
  assert(s1.ok());
  rocksdb::Options opts1;
  opts1.create_if_missing = true;
  rocksdb::Status s2 = rocksdb::DB::Open(opts1, raftPath, &raftDB_);
  assert(s2.ok());
}

Engines::~Engines() {}

bool Engines::Close() {
  delete kvDB_;
  delete raftDB_;
}

bool Engines::Destory() {}

//
// write batch to kvDB_
//
bool Engines::WriteKV(rocksdb::WriteBatch& batch) {
  rocksdb::Status s = kvDB_->Write(rocksdb::WriteOptions(), &batch);
  assert(s.ok());
}

//
// write batch to raftDB_
//
bool Engines::WriteRaft(rocksdb::WriteBatch& batch) {
  rocksdb::Status s = raftDB_->Write(rocksdb::WriteOptions(), &batch);
  assert(s.ok());
}

}  // namespace kvserver
