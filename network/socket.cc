#include "socket.h"
#include <cassert>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <netinet/tcp.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

std::atomic<std::size_t> Socket::sid_;

Socket::Socket() : localSock_(INVALID_SOCKET), epollOut_(false), invalid_(false)
{
	++sid_;

	std::size_t expect = 0;
	sid_.compare_exchange_strong(expect, 1);

	id_ = sid_;
}

Socket::~Socket()
{
	CloseSocket(localSock_);
}

bool Socket::OnError()
{
	bool expect = false;
	if (invalid_.compare_exchange_strong(expect, true)) {
		return true;
	}

	return false;
}

int Socket::CreateTCPSocket()
{
	return ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

void Socket::CloseSocket(int &sock)
{
	if (sock != INVALID_SOCKET) {
		::shutdown(sock, SHUT_RDWR);
		::close(sock);
		sock = INVALID_SOCKET;
	}
}

void Socket::SetNonBlock(int sock, bool nonblock)
{
	int flag = ::fcntl(sock, F_GETFL, 0);
	assert(flag >= 0 && "Non Block failed");

	if (nonblock)
		flag = ::fcntl(sock, F_SETFL, flag | O_NONBLOCK);
	else
		flag = ::fcntl(sock, F_SETFL, flag & ~O_NONBLOCK);
}

void Socket::SetNodelay(int sock)
{
	int nodelay = 1;
	::setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&nodelay, sizeof(int));
}

void Socket::SetSndBuf(int sock, socklen_t winsize)
{
	::setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (const char *)&winsize,
		     sizeof(winsize));
}

void Socket::SetRcvBuf(int sock, socklen_t winsize)
{
	::setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (const char *)&winsize,
		     sizeof(winsize));
}

void Socket::SetReuseAddr(int sock)
{
	int reuse = 1;
	::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));
}

bool Socket::GetLocalAddr(int sock, SocketAddr &addr)
{
	sockaddr_in localAddr;
	socklen_t len = sizeof(localAddr);

	if (0 == ::getsockname(sock, (struct sockaddr *)&localAddr, &len)) {
		addr.Init(localAddr);
	} else {
		return false;
	}

	return true;
}

bool Socket::GetPeerAddr(int sock, SocketAddr &addr)
{
	sockaddr_in remoteAddr;
	socklen_t len = sizeof(remoteAddr);

	if (0 == ::getpeername(sock, (struct sockaddr *)&remoteAddr, &len)) {
		addr.Init(remoteAddr);
	} else {
		return false;
	}

	return true;
}
