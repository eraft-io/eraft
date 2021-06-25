#ifndef ERAFT_KV_RAFTSTORAGE_H_
#define ERAFT_KV_RAFTSTORAGE_H_

#include <Kv/Config.h>
#include <Kv/Engines.h>
#include <Kv/Router.h>
#include <Kv/ServerTransport.h>

#include <eraftio/metapb.pb.h>

namespace kvserver
{

struct GlobalContext
{

    Config *cfg_;

    Engines *engine_;

    metapb::Store storeMeta_;

    // TODO: router
    Router *router_;



    // TODO: transport

    // TODO: Scheduler Client

    
};


} // namespace kvserver


#endif