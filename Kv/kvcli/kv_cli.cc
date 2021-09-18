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

#include <Kv/config.h>
#include <Kv/raft_client.h>
#include <eraftio/helloworld.grpc.pb.h>
#include <eraftio/kvrpcpb.grpc.pb.h>
#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>

using namespace kvserver;

//
// Usage
// get key: ./kv_cli  [leader_ip] [op] [cf] [key]
//
// put key: ./kv_cli  [leader_ip] [op] [cf] [key] [value]
//

int main(int argc, char** argv) {
  std::shared_ptr<Config> conf =
      std::make_shared<Config>(std::string(argv[1]), std::string(argv[2]), 0);

  std::string helpStr = std::string(
      "Usage get key :./ \n \
                        kv_cli[leader_ip][op][cf][key] \n  \
                        put key \n \
      :./ kv_cli[leader_ip][op][cf][key][value] ");

  std::string reqType = std::string(argv[2]);
  std::shared_ptr<RaftClient> raftClient = std::make_shared<RaftClient>(conf);

  if (reqType == "put") {
    kvrpcpb::RawPutRequest request;
    request.mutable_context()->set_region_id(1);
    request.set_cf(std::string(argv[3]));
    request.set_key(std::string(argv[4]));
    request.set_value(std::string(argv[5]));
    request.set_type(1);
    raftClient->PutRaw(std::string(argv[1]), request);
  } else if (reqType == "delete") {
    kvrpcpb::RawPutRequest request;
    request.mutable_context()->set_region_id(1);
    request.set_cf(std::string(argv[3]));
    request.set_key(std::string(argv[4]));
    request.set_value("#");
    request.set_type(2);
    raftClient->PutRaw(std::string(argv[1]), request);
  } else if (reqType == "get") {
    kvrpcpb::RawGetRequest request;
    request.mutable_context()->set_region_id(1);
    request.set_cf(std::string(argv[3]));
    request.set_key(std::string(argv[4]));
    std::string value = raftClient->GetRaw(std::string(argv[1]), request);
    std::cout << "Get value: " << value << std::endl;
  } else if (reqType == "change_leader") {
    raft_cmdpb::TransferLeaderRequest transLeaderReq;
    metapb::Peer* pr = new metapb::Peer();
    pr->set_id(std::atoi(argv[3]));
    transLeaderReq.set_allocated_peer(pr);
    raftClient->TransferLeader(std::string(argv[1]), transLeaderReq);
  } else if (reqType == "config_change") {
    raft_cmdpb::ChangePeerRequest confChange;
    std::string confChangeType = std::string(argv[3]);
    if (confChangeType == "add")
      confChange.set_change_type(eraftpb::AddNode);
    else if (confChangeType == "remove")
      confChange.set_change_type(eraftpb::RemoveNode);
    metapb::Peer* pr = new metapb::Peer();
    pr->set_addr(std::string(argv[4]));
    pr->set_id(std::atoi(argv[5]));
    pr->set_store_id(std::atoi(argv[5]));
    confChange.set_allocated_peer(pr);
    raftClient->PeerConfChange(std::string(argv[1]), confChange);

  } else if (reqType == "split") {
    //
    raft_cmdpb::SplitRequest splitReuest;
    splitReuest.set_split_key(argv[3]);
    splitReuest.set_new_region_id(std::atoi(argv[4]));
    raftClient->SplitRegion(std::string(argv[1]), splitReuest);
  } else {
    std::cerr << helpStr << std::endl;
  }

  return 0;
}
