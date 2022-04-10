#include "server.h"
#include "listen_socket.h"
#include "net_thread_pool.h"
#include "stream_socket.h"
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <signal.h>
#include <sys/resource.h>

using Internal::NetThreadPool;

void Server::IntHandler(int signum)
{
	if (Server::Instance() != NULL)
		Server::Instance()->Terminate();
}

void Server::HupHandler(int signum)
{
	if (Server::Instance() != NULL)
		Server::Instance()->reloadCfg_ = true;
}

Server *Server::sinstance_ = nullptr;

std::set<int> Server::slistenSocks_;

Server::Server() : bTerminate_(false), reloadCfg_(false)
{
	if (sinstance_ == NULL)
		sinstance_ = this;
	else
		::abort();
}

Server::~Server()
{
	sinstance_ = NULL;
}

bool Server::_RunLogic()
{
	return tasks_.DoMsgParse();
}

bool Server::TCPBind(const SocketAddr &addr, int tag)
{
	using Internal::ListenSocket;

	auto s(std::make_shared<ListenSocket>(tag));

	if (s->Bind(addr)) {
		slistenSocks_.insert(s->GetSocket());
		return true;
	}

	return false;
}

void Server::TCPReconnect(const SocketAddr &peer, int tag)
{
	// TODO
}

void Server::TCPConnect(const SocketAddr &peer, int tag)
{
}

void Server::TCPConnect(const SocketAddr &peer, const std::function<void()> &cb, int tag)
{
}

void Server::MainLoop(bool daemon)
{
	struct sigaction sig;
	::memset(&sig, 0, sizeof(sig));
	sig.sa_handler = &Server::IntHandler;
	sigaction(SIGINT, &sig, NULL);
	sigaction(SIGQUIT, &sig, NULL);
	sigaction(SIGABRT, &sig, NULL);
	sigaction(SIGTERM, &sig, NULL);
	sig.sa_handler = &Server::HupHandler;
	sigaction(SIGHUP, &sig, NULL);

	sig.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sig, NULL);

	::pthread_atfork(nullptr, nullptr, AtForkHandler);

	::srand(static_cast<unsigned int>(time(NULL)));
	::srandom(static_cast<unsigned int>(time(NULL)));

	if (daemon) {
		::daemon(1, 0);
	}

	if (NetThreadPool::Instance().StartAllThreads() && _Init()) {
		std::cout << "run server success!" << std::endl;
		while (!bTerminate_) {
			if (reloadCfg_) {
				ReloadConfig();
				reloadCfg_ = false;
			}

			if (!_RunLogic())
				std::this_thread::sleep_for(
					std::chrono::microseconds(100));
		}
	}

	tasks_.Clear();
	_Recycle();
	NetThreadPool::Instance().StopAllThreads();
	ThreadPool::Instance().JoinAll();
}

std::shared_ptr<StreamSocket> Server::_OnNewConnection(int tcpsock, int tag)
{
	return std::shared_ptr<StreamSocket>(nullptr);
}

void Server::NewConnection(int sock, int tag, const std::function<void()> &cb)
{
	if (sock == INVALID_SOCKET)
		return;

	auto conn = _OnNewConnection(sock, tag);

	if (!conn) {
		Socket::CloseSocket(sock);
		return;
	}

	conn->SetOnDisconnect(cb);

	if (NetThreadPool::Instance().AddSocket(conn, EventTypeRead | EventTypeWrite))
		tasks_.AddTask(conn);
}

void Server::AtForkHandler()
{
	for (auto sock : slistenSocks_) {
		close(sock);
	}
}

void Server::DelListenSock(int sock)
{
	if (sock == INVALID_SOCKET)
		return;

	auto n = slistenSocks_.erase(sock);

	if (n != 1) {
		// Failed Del
	} else {
		// Success Del
	}
}
