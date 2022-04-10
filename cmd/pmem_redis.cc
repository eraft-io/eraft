//
//  TiRedis.cc
//

#include "pmem_redis.h"
#include "client.h"
#include "command.h"
#include "config.h"
#include "executor.h"
#include "socket.h"
#include <iostream>

PMemRedis::PMemRedis() : port_(0)
{
}

PMemRedis::~PMemRedis()
{
}

std::shared_ptr<StreamSocket> PMemRedis::_OnNewConnection(int connfd, int tag)
{
	// new connection comming
	SocketAddr peer;
	Socket::GetPeerAddr(connfd, peer);

	auto cli(std::make_shared<Client>());
	if (!cli->Init(connfd, peer))
		cli.reset();
	return cli;
}

bool PMemRedis::_Init()
{

	SocketAddr addr(g_config.listenAddr);
	std::vector<std::string> engine_params{g_config.dbPath};
	CommandTable::Init();
	Executor::Init(engine_params);
	if (!Server::TCPBind(addr, 1)) {
		return false;
	}
	std::cout << "server listen on: " << g_config.listenAddr << std::endl;

	return true;
}

bool PMemRedis::_RunLogic()
{
	return Server::_RunLogic();
}

bool PMemRedis::_Recycle()
{
	// delete Executor::engine_;
	// free resources
}

int main(int ac, char *av[])
{
	PMemRedis svr;

	if (!LoadServerConfig(std::string(av[1]).c_str(), g_config)) {
		std::cerr << "Load config file pmem_redis.toml failed!\n";
		return -2;
	}

	svr.MainLoop(false);

	return 0;
}
