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
#include "estatus.h"
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
  friend class ShardGroupCommandHandler;

 private:
  PacketLength _HandlePacket(const char *msg, std::size_t len) override;

  UnboundedBuffer reply_;

  ProtoParser parser_;

  std::map<std::string, std::unique_ptr<ERaftKv::Stub> > kv_stubs_;

  std::map<std::string, std::unique_ptr<ERaftKv::Stub> > meta_stubs_;

  std::string leader_addr_;

  eraftkv::ClusterConfigChangeResp cluster_conf_;

  std::string meta_addrs_;

 public:
  Client(std::string meta_addrs);

  /**
   * @brief Get the kv shard group leader address
   *
   * @param partion_key
   * @return std::string
   */
  std::string GetShardLeaderAddr(std::string partion_key);

  /**
   * @brief Get the meta server leader address
   *
   * @return std::string
   */
  std::string GetMetaLeaderAddr();

  EStatus SyncClusterConfig();

  void _Reset();

  void OnConnect() override;
};
