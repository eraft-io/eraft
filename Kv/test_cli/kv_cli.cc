// MIT License

// Copyright (c) 2021 Colin

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <eraftio/helloworld.grpc.pb.h>
#include <eraftio/kvrpcpb.grpc.pb.h>
#include <Kv/raft_client.h>
#include <Kv/config.h>

using namespace kvserver;

int main(int argc, char** argv) {

  std::shared_ptr<Config> conf = std::make_shared<Config>(std::string(argv[1]), 
    std::string(argv[2]), std::stoi(std::string(argv[3])));
  
  std::shared_ptr<RaftClient> raftClient = std::make_shared<RaftClient>(conf);
  kvrpcpb::RawPutRequest request;
  request.mutable_context()->set_region_id(1);
  request.set_cf(std::string(argv[2]));
  request.set_value(std::string(argv[2]));
  request.set_key(std::string(argv[2]));
  raftClient->PutRaw(std::string(argv[1]), request);

  return 0;
}
