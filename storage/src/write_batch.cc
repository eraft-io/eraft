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

#include <storage/write_batch.h>

namespace storage
{

  WriteBatch::WriteBatch() {}

  WriteBatch::~WriteBatch() {}

  bool WriteBatch::Put(std::string key, std::string value)
  {
    KVPair kvPair(key, value);
    this->items_.emplace_back(std::make_pair<BacthOpCode, KVPair&>(BacthOpCode::Put, kvPair));
  }

  bool WriteBatch::Delete(std::string key)
  {
    KVPair kvPair(key, "");
    this->items_.emplace_back(std::make_pair<BacthOpCode, KVPair&>(BacthOpCode::Delete, kvPair));
  }

  std::deque<std::pair<BacthOpCode, KVPair> > WriteBatch::GetItems()
  {
    return this->items_;
  }

} // namespace storage
