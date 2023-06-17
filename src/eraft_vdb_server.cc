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

ERaftVdbServer::ERaftVdbServer() : port_(0) {}

ERaftVdbServer::~ERaftVdbServer() {}

std::shared_ptr<StreamSocket> ERaftVdbServer::_OnNewConnection(int connfd,
                                                               int tag) {
  SocketAddr peer;
  Socket::GetPeerAddr(connfd, peer);

  auto cli(std::make_shared<Client>(KV_SERVER_ADDRS));
  if (!cli->Init(connfd, peer))
    cli.reset();
  return cli;
}

bool ERaftVdbServer::_Init() {
  SocketAddr addr("0.0.0.0:12306");
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

int main(int argc, char *argv[]) {
  ERaftVdbServer svr;
  svr.MainLoop(false);

  return 0;
}
