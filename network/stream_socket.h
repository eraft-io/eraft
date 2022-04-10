#pragma once

#include "async_buffer.h"
#include "socket.h"
#include <sys/socket.h>
#include <sys/types.h>

using PacketLength = int32_t;

class StreamSocket : public Socket {
	friend class SendThread;

public:
	StreamSocket();
	~StreamSocket();

	bool Init(int localfd, const SocketAddr &peer);
	SocketType GetSocketType() const
	{
		return SocketType_Stream;
	}

public:
	int Recv();

public:
	bool SendPacket(const void *, std::size_t);
	bool SendPacket(Buffer &bf);
	bool SendPacket(AttachedBuffer &abf);
	bool SendPacket(UnboundedBuffer &ubf);

	bool OnReadable();
	bool OnWriteable();
	bool OnError();

	bool DoMsgParse();

	void SetOnDisconnect(const std::function<void()> &cb = std::function<void()>())
	{
		onDisconnect_ = cb;
	}

	bool Send();

	const SocketAddr &GetPeerAddr() const
	{
		return peerAddr_;
	}

protected:
	SocketAddr peerAddr_;

private:
	std::function<void()> onDisconnect_;

	// send buf to peer
	int _Send(const BufferSequence &bf);
	virtual PacketLength _HandlePacket(const char *msg, std::size_t len) = 0;

	enum {
		TIMEOUTSOCKET = 0,
		ERRORSOCKET = -1,
		EOFSOCKET = -2,
	};

	Buffer recvBuf_;

	AsyncBuffer sendBuf_;
};
