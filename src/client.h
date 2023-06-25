/**
 * @file client.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-06-17
 *
 * @copyright Copyright (c) 2023
 *
 */


#pragma once

#include <grpcpp/grpcpp.h>

#include "command_handler.h"
#include "eraftkv.grpc.pb.h"
#include "eraftkv.pb.h"
#include "proto_parser.h"
#include "stream_socket.h"
#include "unbounded_buffer.h"

using eraftkv::ERaftKv;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

class Client : public StreamSocket {
  friend class InfoCommandHandler;
  friend class SetCommandHandler;
  friend class GetCommandHandler;
  friend class UnKnowCommandHandler;

 private:
  PacketLength _HandlePacket(const char *msg, std::size_t len) override;

  UnboundedBuffer reply_;

  ProtoParser parser_;

  std::map<std::string, std::unique_ptr<ERaftKv::Stub> > stubs_;

  std::string leader_addr_;

 public:
  Client(std::string kv_addrs);

  std::string GetLeaderAddr();

  void _Reset();

  void OnConnect() override;
};
