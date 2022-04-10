#include <errno.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include "net_thread_pool.h"
#include "server.h"
#include "stream_socket.h"

using std::size_t;

StreamSocket::StreamSocket()
{
}

StreamSocket::~StreamSocket()
{
}

bool StreamSocket::Init(int fd, const SocketAddr &peer)
{
	if (fd < 0)
		return false;

	peerAddr_ = peer;
	localSock_ = fd;
	SetNonBlock(localSock_);

	return true;
}

int StreamSocket::Recv()
{
	if (recvBuf_.Capacity() == 0) {
		recvBuf_.InitCapacity(64 * 1024); // 64k
	}

	BufferSequence buffers;
	recvBuf_.GetSpace(buffers);
	if (buffers.count == 0) {
		// Recv buffer is full
		return 0;
	}

	int ret = static_cast<int>(
		::readv(localSock_, buffers.buffers, static_cast<int>(buffers.count)));
	if (ret == ERRORSOCKET && (EAGAIN == errno || EWOULDBLOCK == errno))
		return 0;

	if (ret > 0)
		recvBuf_.AdjustWritePtr(ret);

	return (0 == ret) ? EOFSOCKET : ret;
}

int StreamSocket::_Send(const BufferSequence &bf)
{
	auto total = bf.TotalBytes();
	if (total == 0)
		return 0;

	int ret = static_cast<int>(
		::writev(localSock_, bf.buffers, static_cast<int>(bf.count)));
	if (ERRORSOCKET == ret && (EAGAIN == errno || EWOULDBLOCK == errno)) {
		epollOut_ = true;
		ret = 0;
	} else if (ret > 0 && static_cast<size_t>(ret) < total) {
		epollOut_ = true;
	} else if (static_cast<size_t>(ret) == total) {
		epollOut_ = false;
	}

	return ret;
}

bool StreamSocket::SendPacket(const void *data, size_t bytes)
{
	if (data && bytes > 0)
		sendBuf_.Write(data, bytes);

	return true;
}

bool StreamSocket::SendPacket(Buffer &bf)
{
	return SendPacket(bf.ReadAddr(), bf.ReadableSize());
}

bool StreamSocket::SendPacket(AttachedBuffer &af)
{
	return SendPacket(af.ReadAddr(), af.ReadableSize());
}

bool StreamSocket::SendPacket(UnboundedBuffer &ubf)
{
	return SendPacket(ubf.ReadAddr(), ubf.ReadableSize());
}

bool StreamSocket::OnReadable()
{
	int nBytes = StreamSocket::Recv();

	if (nBytes < 0) {
		// failed
		Internal::NetThreadPool::Instance().DisableRead(shared_from_this());
		return false;
	}

	return true;
}

bool StreamSocket::Send()
{
	if (epollOut_)
		return true;

	BufferSequence bf;
	sendBuf_.ProcessBuffer(bf);

	size_t total = bf.TotalBytes();
	if (total == 0)
		return true;

	int nSent = _Send(bf);

	if (nSent > 0) {
		sendBuf_.Skip(nSent);
	}

	if (epollOut_) {
		Internal::NetThreadPool::Instance().EnableWrite(shared_from_this());
		// register write event
	}

	return nSent >= 0;
}

// EPOLLOUT
bool StreamSocket::OnWriteable()
{
	BufferSequence bf;
	sendBuf_.ProcessBuffer(bf);

	size_t total = bf.TotalBytes();
	int nSent = 0;
	if (total > 0) {
		// send to client
		nSent = _Send(bf);
		if (nSent > 0)
			sendBuf_.Skip(nSent);
	} else {
		epollOut_ = false;
	}

	if (!epollOut_) {
		Internal::NetThreadPool::Instance().DisableWrite(shared_from_this());
	}

	return nSent >= 0;
}

bool StreamSocket::OnError()
{
	if (Socket::OnError()) {
		if (onDisconnect_)
			onDisconnect_();

		return true;
	}

	return false;
}

// deal with message
bool StreamSocket::DoMsgParse()
{
	bool busy = false;
	while (!recvBuf_.IsEmpty()) {
		BufferSequence datum;
		recvBuf_.GetDatum(datum, recvBuf_.ReadableSize());

		AttachedBuffer af(datum);
		auto bodyLen = _HandlePacket(af.ReadAddr(), af.ReadableSize());
		if (bodyLen > 0) {
			busy = true;
			recvBuf_.AdjustReadPtr(bodyLen);
		} else {
			break;
		}
	}

	return busy;
}
