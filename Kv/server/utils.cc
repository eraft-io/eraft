#include <Kv/utils.h>

namespace kvserver
{

const std::vector<uint8_t> Assistant::kLocalPrefix = { 0x01 };

const std::vector<uint8_t> Assistant::kRegionRaftPrefix = { 0x02 };

const std::vector<uint8_t> Assistant::kRegionMetaPrefix = { 0x03 };

const uint8_t Assistant::kRegionRaftPrefixLen = 11;

const uint8_t Assistant::kRegionRaftLogLen = 19;

const uint64_t Assistant::kInvalidID = 0;

const std::vector<uint8_t> Assistant::kRaftLogSuffix = { 0x01 };

const std::vector<uint8_t> Assistant::kRaftStateSuffix = { 0x02 };

const std::vector<uint8_t> Assistant::kApplyStateSuffix = { 0x03 };

const std::vector<uint8_t> Assistant::kRegionStateSuffix = { 0x01 };

const std::vector<uint8_t> Assistant::MinKey = {};

const std::vector<uint8_t> Assistant::MaxKey = { 255 };

const std::vector<uint8_t> Assistant::LocalMinKey = { 0x01 };

const std::vector<uint8_t> Assistant::LocalMaxKey = { 0x02 };

// need to scan [RegionMetaMinKey, RegionMetaMaxKey]
const std::vector<uint8_t> Assistant::RegionMetaMinKey = { 0x01, 0x03 };

const std::vector<uint8_t> Assistant::RegionMetaMaxKey = { 0x01, 0x04 };

const std::vector<uint8_t> Assistant::PrepareBootstrapKey = { 0x01, 0x01 };

const std::vector<uint8_t> Assistant::StoreIdentKey = { 0x01, 0x02 };

const std::string Assistant::CfDefault = "default";
const std::string Assistant::CfWrite = "write";
const std::string Assistant::CfLock = "lock";

const std::vector<std::string> Assistant::CFs = { CfDefault, CfWrite, CfLock };

Assistant* Assistant::instance_ = nullptr;

}