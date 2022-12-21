// Copyright 2022 © Colin
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <string.h>
#include <iostream>
#include <string>
#include <stdio.h>
#include <signal.h>
#include <condition_variable>
#include <mutex>
#include <iostream>
#include <unistd.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <algorithm>
#include "parser/parser.h"
#include "database/dbms.h"
#include "../network/client.h"
#include "../network/server.h"
#include "../network/socket.h"
#include "parser/defs.h"
#include "parser/parser.h"

extern "C" char run_parser(const char *input);

Uhpsqld::Uhpsqld() : port_(0) {}

Uhpsqld::~Uhpsqld() {}

std::shared_ptr<StreamSocket> Uhpsqld::_OnNewConnection(int connfd, int tag) {
  SocketAddr peer;
  Socket::GetPeerAddr(connfd, peer);

  auto cli(std::make_shared<Client>());
  if (!cli->Init(connfd, peer)) cli.reset();
  return cli;
}

bool Uhpsqld::_Init() {
  SocketAddr addr(DEMO_SERVER_ADDR);
  if (!Server::TCPBind(addr, 1)) {
    return false;
  }
  return true;
}

bool Uhpsqld::_RunLogic() { return Server::_RunLogic(); }

bool Uhpsqld::_Recycle() {
  return true;
}

int main(int argc, char *argv[]) {
  Uhpsqld svr;
  std::cout << "WELCOME TO TINY DB!" << std::endl;
  run_parser("USE db;");
  svr.MainLoop(false);

  return 0;
}
