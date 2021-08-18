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

#ifndef ERAFT_KV_ENGINES_H_
#define ERAFT_KV_ENGINES_H_

#include <rocksdb/db.h>
#include <rocksdb/write_batch.h>

#include <cassert>
#include <memory>

namespace kvserver {

class Engines {
 public:
  Engines(std::string kvPath, std::string raftPath);

  ~Engines();

  bool WriteKV(rocksdb::WriteBatch& batch);

  bool WriteRaft(rocksdb::WriteBatch& batch);

  bool Close();

  bool Destory();

  rocksdb::DB* kvDB_;

  std::string kvPath_;

  rocksdb::DB* raftDB_;

  std::string raftPath_;
};

}  // namespace kvserver

#endif