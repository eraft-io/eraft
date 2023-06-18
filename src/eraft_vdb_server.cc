/**
 * @file eraft_vdb_server.cc
 * @author ERaftGroup
 * @brief
 * @version 0.1
 * @date 2023-06-17
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "eraft_vdb_server.h"

#include "client.h"
#include "server.h"
#include "socket.h"

ERaftVdbServer::ERaftVdbServer(std::string addr, std::string kv_svr_addrs)
    : port_(0), addr_(addr), kv_svr_addrs_(kv_svr_addrs) {}

ERaftVdbServer::~ERaftVdbServer() {}

std::shared_ptr<StreamSocket> ERaftVdbServer::_OnNewConnection(int connfd,
                                                               int tag) {
  SocketAddr peer;
  Socket::GetPeerAddr(connfd, peer);

  auto cli(std::make_shared<Client>(kv_svr_addrs_));
  if (!cli->Init(connfd, peer))
    cli.reset();
  return cli;
}

bool ERaftVdbServer::_Init() {
  SocketAddr addr(addr_);
  if (!Server::TCPBind(addr, 1)) {
    return false;
  }
  return true;
}

bool ERaftVdbServer::_RunLogic() {
  return Server::_RunLogic();
}

bool ERaftVdbServer::_Recycle() {
  return true;
}

/**
 * @brief
 *
 * @param argc
 * @param argv
 * @return int
 */
int main(int argc, char *argv[]) {
  std::string    addr = std::string(argv[1]);
  std::string    kv_svr_addrs = std::string(argv[2]);
  ERaftVdbServer svr(addr, kv_svr_addrs);
  svr.MainLoop(false);
  return 0;
}
