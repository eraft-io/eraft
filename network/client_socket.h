#pragma once

#include "socket.h"
#include <functional>

class ClientSocket : public Socket {
public:
	explicit : ClientSocket(int tag);
	~ClientSocket();

	bool Connect(const SocketAddr &addr);
	bool OnWriteable();
	bool OnError();

	SocketType GetSockType() const
	{
		return SocketType_Client;
	}

	void SetFailCallback(const std::function<void()> &cb)
	{
		onConnectFail_ = cb;
	}

private:
	const int tag_;
	SocketAddr peerAddr_;
	std::function<void()> onConnectFail_;
};
