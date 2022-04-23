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

#include <network/raft_encode_assistant.h>

namespace network {

RaftEncodeAssistant *RaftEncodeAssistant::instance_ = nullptr;

const std::vector<uint8_t> RaftEncodeAssistant::kLocalPrefix = {0x01};

const std::vector<uint8_t> RaftEncodeAssistant::kRegionRaftPrefix = {0x02};

const std::vector<uint8_t> RaftEncodeAssistant::kRegionMetaPrefix = {0x03};

const uint8_t RaftEncodeAssistant::kRegionRaftPrefixLen = 11;

const uint8_t RaftEncodeAssistant::kRegionRaftLogLen = 19;

const uint64_t RaftEncodeAssistant::kInvalidID = 0;

const std::vector<uint8_t> RaftEncodeAssistant::kRaftLogSuffix = {0x01};

const std::vector<uint8_t> RaftEncodeAssistant::kRaftStateSuffix = {0x02};

const std::vector<uint8_t> RaftEncodeAssistant::kApplyStateSuffix = {0x03};

const std::vector<uint8_t> RaftEncodeAssistant::kRegionStateSuffix = {0x01};

const std::vector<uint8_t> RaftEncodeAssistant::MinKey = {};

const std::vector<uint8_t> RaftEncodeAssistant::MaxKey = {255};

const std::vector<uint8_t> RaftEncodeAssistant::LocalMinKey = {0x01};

const std::vector<uint8_t> RaftEncodeAssistant::LocalMaxKey = {0x02};

// need to scan [RegionMetaMinKey, RegionMetaMaxKey]
const std::vector<uint8_t> RaftEncodeAssistant::RegionMetaMinKey = {0x01, 0x03};

const std::vector<uint8_t> RaftEncodeAssistant::RegionMetaMaxKey = {0x01, 0x04};

const std::vector<uint8_t> RaftEncodeAssistant::PrepareBootstrapKey = {0x01,
                                                                       0x01};

const std::vector<uint8_t> RaftEncodeAssistant::StoreIdentKey = {0x01, 0x02};

const std::string RaftEncodeAssistant::CfDefault = "default";
const std::string RaftEncodeAssistant::CfWrite = "write";
const std::string RaftEncodeAssistant::CfLock = "lock";

}  // namespace network
