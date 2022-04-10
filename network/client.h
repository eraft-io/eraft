#pragma once

#include "proto_parser.h"
#include "stream_socket.h"
#include "unbounded_buffer.h"

class Client : public StreamSocket {
private:
	PacketLength _HandlePacket(const char *msg, std::size_t len) override;

	UnboundedBuffer reply_;

	ProtoParser parser_;

public:
	Client();

	void _Reset();

	void OnConnect() override;
};
