#pragma once

#include "socket.h"

namespace Internal
{

class ListenSocket : public Socket {
	static const int LISTENQ;

public:
	explicit ListenSocket(int tag);
	~ListenSocket();

	SocketType GetSocketType() const
	{
		return SocketType_Listen;
	}

	bool Bind(const SocketAddr &addr);
	bool OnReadable();
	bool OnWriteable();
	bool OnError();

private:
	int _Accept();
	sockaddr_in addrClient_;
	unsigned short localPort_;
	const int tag_;
};

};
