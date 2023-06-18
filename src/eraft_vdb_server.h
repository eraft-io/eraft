/**
 * @file eraft_vdb_server.h
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-06-17
 *
 * @copyright Copyright (c) 2023
 *
 */
#pragma once

#include "server.h"

#define KV_SERVER_ADDRS "127.0.0.1:8088,127.0.0.1:8089,127.0.0.1:8090"

class ERaftVdbServer : public Server {
 public:
  ERaftVdbServer(std::string addr, std::string kv_svr_addrs);
  ~ERaftVdbServer();

 private:
  std::shared_ptr<StreamSocket> _OnNewConnection(int fd, int tag) override;

  bool _Init() override;
  bool _RunLogic() override;
  bool _Recycle() override;

  unsigned short port_;

  std::string addr_;

  std::string kv_svr_addrs_;
};
