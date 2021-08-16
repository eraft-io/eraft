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

#ifndef ERAFT_KV_STORAGE_H_
#define ERAFT_KV_STORAGE_H_

#include <eraftio/kvrpcpb.pb.h>
#include <rocksdb/db.h>
#include <stdint.h>

#include <string>
#include <vector>

namespace kvserver {

enum class OpType {
  Get,
  Put,
  Delete,
};

struct Put {
  Put(std::string key, std::string value, std::string cf) {
    this->key_ = key;
    this->value_ = value;
    this->cf_ = cf;
  }

  std::string key_;
  std::string value_;
  std::string cf_;
};

struct Delete {
  Delete(std::string key, std::string cf) {
    this->key = key;
    this->cf = cf;
  }

  std::string key;
  std::string cf;
};

struct Modify {
  Modify(void* data, OpType ot) {
    this->data_ = data;
    this->ot_ = ot;
  }

  std::string Key() {
    switch (this->ot_) {
      case OpType::Put: {
        struct Put* pt = (struct Put*)this->data_;
        return pt->key_;
      }
      case OpType::Delete: {
        struct Delete* dt = (struct Delete*)this->data_;
        return dt->key;
      }
      default:
        break;
    }
    return "";
  }

  std::string Value() {
    if (ot_ == OpType::Put) {
      struct Put* pt = (struct Put*)this->data_;
      return pt->value_;
    }
    return "";
  }

  std::string Cf() {
    switch (this->ot_) {
      case OpType::Put: {
        struct Put* pt = (struct Put*)this->data_;
        return pt->cf_;
      }
      case OpType::Delete: {
        struct Delete* dt = (struct Delete*)this->data_;
        return dt->key;
      }
      default:
        break;
    }
    return "";
  }

  OpType ot_;

  void* data_;
};

class StorageReader {
 public:
  virtual ~StorageReader(){};

  virtual std::string GetFromCF(std::string cf, std::string key) = 0;

  virtual rocksdb::Iterator* IterCF(std::string cf) = 0;

  virtual void Close() = 0;
};

class Storage {
 public:
  virtual ~Storage() {}

  virtual bool Start() = 0;

  virtual bool Write(const kvrpcpb::Context& ctx,
                     const kvrpcpb::RawPutRequest* put) = 0;

  virtual StorageReader* Reader(const kvrpcpb::Context& ctx) = 0;
};

}  // namespace kvserver

#endif