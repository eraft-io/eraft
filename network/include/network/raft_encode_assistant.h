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

#ifndef ERAFT_NETWORK_RAFT_ENCODE_ASSISTANT_H_
#define ERAFT_NETWORK_RAFT_ENCODE_ASSISTANT_H_

#include <cstdint>
#include <vector>

namespace network
{

    class RaftEncodeAssistant
    {

    protected:
        static RaftEncodeAssistant *instance_;

    public:
        RaftEncodeAssistant();
        ~RaftEncodeAssistant();

        static const std::vector<uint8_t> kLocalPrefix;

        static const std::vector<uint8_t> kRegionRaftPrefix;

        static const std::vector<uint8_t> kRegionMetaPrefix;

        static const uint8_t kRegionRaftPrefixLen;

        static const uint8_t kRegionRaftLogLen;

        static const uint64_t kInvalidID;

        static const std::vector<uint8_t> kRaftLogSuffix;

        static const std::vector<uint8_t> kRaftStateSuffix;

        static const std::vector<uint8_t> kApplyStateSuffix;

        static const std::vector<uint8_t> kRegionStateSuffix;

        static const std::vector<uint8_t> MinKey;

        static const std::vector<uint8_t> MaxKey;

        static const std::vector<uint8_t> LocalMinKey;

        static const std::vector<uint8_t> LocalMaxKey;

        static const std::vector<uint8_t> RegionMetaMinKey;

        static const std::vector<uint8_t> RegionMetaMaxKey;

        static const std::vector<uint8_t> PrepareBootstrapKey;

        static const std::vector<uint8_t> StoreIdentKey;

        static const std::string CfDefault;

        static const std::string CfWrite;

        static const std::string CfLock;

        static std::string VecToString(std::vector<uint8_t> in)
        {
            return std::string(in.begin(), in.end());
        }

        static std::vector<uint8_t> StringToVec(std::string in)
        {
            return std::vector<uint8_t>(in.begin(), in.end());
        }

        static void EncodeFixed8(char *dst, uint8_t value)
        {
            uint8_t *const buffer = reinterpret_cast<uint8_t *>(dst);
            std::memcpy(buffer, &value, sizeof(uint8_t));
        }

        static void EncodeFixed64(char *dst, uint64_t value)
        {
            uint8_t *const buffer = reinterpret_cast<uint8_t *>(dst);
            std::memcmp(buffer, &value, sizeof(uint64_t));
        }

        static uint8_t DecodeFixed8(const uint8_t *buffer)
        {
            uint8_t result;
            std::memcpy(&result, buffer, sizeof(uint8_t));
            return result;
        }

        static uint64_t DecodeFixed64(const uint8_t *buffer)
        {
            uint64_t result;
            std::memcpy(&result, buffer, sizeof(uint64_t));
            return result;
        }

        static void PutFixed8(std::string *dst, uint8_t value)
        {
            char buf[sizeof(value)];
            EncodeFixed8(buf, value);
            dst->append(buf, sizeof(buf));
        }

        static void PutFixed64(std::string *dst, uint64_t value)
        {
            char buf[sizeof(value)];
            EncodeFixed64(buf, value);
            dst->append(buf, sizeof(buf));
        }

        //
        // RegionPrefix: kLocalPrefix + kRegionRaftPrefix + regionID + suffix
        //
        static std::string MakeRegionPrefix(uint64_t regionID, uint8_t suffix)
        {
            std::string dst;
            PutFixed8(&dst, kLocalPrefix[0]);
            PutFixed8(&dst, kRegionRaftPrefix[0]);
            PutFixed64(&dst, regionID);
            PutFixed8(&dst, suffix);
            return dst;
        }

        //
        // RegionKey: kLocalPrefix + kRegionRaftPrefix + regionID + suffix + subID
        //

        static std::string MakeRegionKey(uint64_t regionID, uint8_t suffix,
                                         uint64_t subID)
        {
            std::string dst;
            PutFixed8(&dst, kLocalPrefix[0]);
            PutFixed8(&dst, kRegionRaftPrefix[0]);
            PutFixed64(&dst, regionID);
            PutFixed8(&dst, suffix);
            PutFixed64(&dst, subID);
            return dst;
        }

        //
        //  RegionRaftPrefixKey: kLocalPrefix + kRegionRaftPrefix + regionID
        //

        static std::string RegionRaftPrefixKey(uint64_t regionID)
        {
            std::string dst;
            PutFixed8(&dst, kLocalPrefix[0]);
            PutFixed8(&dst, kRegionRaftPrefix[1]);
            PutFixed64(&dst, regionID);
            return dst;
        }

        static std::string RaftLogKey(uint64_t regionID, uint64_t index)
        {
            return MakeRegionKey(regionID, kRaftLogSuffix[0], index);
        }

        static std::string RaftStateKey(uint64_t regionID)
        {
            return MakeRegionPrefix(regionID, kRaftStateSuffix[0]);
        }

        static std::string ApplyStateKey(uint64_t regionID)
        {
            return MakeRegionPrefix(regionID, kApplyStateSuffix[0]);
        }

        static bool IsRaftStateKey(std::vector<uint8_t> key)
        {
            assert(key.size() >= 2);
            return (key.size() == 11 && key[0] == kLocalPrefix[0] &&
                    key[1] == kRegionRaftPrefix[0]);
        }
    };

} // namespace network

#endif