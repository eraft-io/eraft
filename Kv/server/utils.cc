// MIT License

// Copyright (c) 2021 Colin

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

#include <Kv/utils.h>

namespace kvserver {

const std::vector<uint8_t> Assistant::kLocalPrefix = {0x01};

const std::vector<uint8_t> Assistant::kRegionRaftPrefix = {0x02};

const std::vector<uint8_t> Assistant::kRegionMetaPrefix = {0x03};

const uint8_t Assistant::kRegionRaftPrefixLen = 11;

const uint8_t Assistant::kRegionRaftLogLen = 19;

const uint64_t Assistant::kInvalidID = 0;

const std::vector<uint8_t> Assistant::kRaftLogSuffix = {0x01};

const std::vector<uint8_t> Assistant::kRaftStateSuffix = {0x02};

const std::vector<uint8_t> Assistant::kApplyStateSuffix = {0x03};

const std::vector<uint8_t> Assistant::kRegionStateSuffix = {0x01};

const std::vector<uint8_t> Assistant::MinKey = {};

const std::vector<uint8_t> Assistant::MaxKey = {255};

const std::vector<uint8_t> Assistant::LocalMinKey = {0x01};

const std::vector<uint8_t> Assistant::LocalMaxKey = {0x02};

// need to scan [RegionMetaMinKey, RegionMetaMaxKey]
const std::vector<uint8_t> Assistant::RegionMetaMinKey = {0x01, 0x03};

const std::vector<uint8_t> Assistant::RegionMetaMaxKey = {0x01, 0x04};

const std::vector<uint8_t> Assistant::PrepareBootstrapKey = {0x01, 0x01};

const std::vector<uint8_t> Assistant::StoreIdentKey = {0x01, 0x02};

const std::string Assistant::CfDefault = "default";
const std::string Assistant::CfWrite = "write";
const std::string Assistant::CfLock = "lock";

const std::vector<std::string> Assistant::CFs = {CfDefault, CfWrite, CfLock};

Assistant* Assistant::instance_ = nullptr;

}  // namespace kvserver