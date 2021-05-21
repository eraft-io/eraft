#ifndef ERAFT_STORAGE_H
#define ERAFT_STORAGE_H

#include <stdint.h>
#include <string>
#include <vector>
#include <eraftio/eraftpb.pb.h>

namespace eraft
{

using EString = std::string;

class StorageInterface
{
public:
    virtual ~StorageInterface() {}

    virtual std::tuple<eraftpb::HardState, eraftpb::ConfState> InitialState() = 0;

    virtual std::vector<eraftpb::Entry> Entries(uint64_t lo, uint64_t hi) = 0;

    virtual uint64_t Term(uint64_t i) = 0;

    virtual uint64_t LastIndex() = 0;

    virtual uint64_t FirstIndex() = 0;

    virtual eraftpb::Snapshot Snapshot() = 0;

};

} // namespace name


#endif