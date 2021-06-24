#ifndef ERAFT_KV_UTIL_H_
#define ERAFT_KV_UTIL_H_

#include <string>
#include <memory>
#include <leveldb/db.h>

namespace kvserver
{

static std::string KeyWithCF(std::string cf, std::string key) {
    return cf + "_"  + key ;
}

static std::string GetCF(std::unique_ptr<leveldb::DB> db, std::string cf, std::string key) {
    std::string res;
    leveldb::Status s = db->Get(leveldb::ReadOptions(), KeyWithCF(cf, key), &res);
    if(s.ok()) {
        return res;
    } else {
        return "";
    }
}


} // namespace kvserver

#endif