#pragma once
#include <string>
#include <map>
#include <vector>
#include <variant>
#include "sslclient.h"

enum WSState {
	HTTP_HEADERS,
	CONNECTED
};

enum OpCode
{
        OP_CONTINUATION = 0x00,
        OP_TEXT = 0x01,
        OP_BINARY = 0x02,
        OP_CLOSE = 0x08,
        OP_PING = 0x09,
        OP_PONG = 0x0a
};

typedef std::map<std::string, std::string> ERLStruct;

class WSClient : public SSLClient
{
	std::string key;
	WSState state;
	uint32_t heartbeat_interval;
	uint32_t shard_id;
	std::map<std::string, std::string> HTTPHeaders;
	bool parseheader(std::string &buffer);
	bool unpack(std::string &buffer, uint32_t offset, bool first = true);
	size_t FillHeader(unsigned char* outbuf, size_t sendlength, OpCode opcode);
public:
        WSClient(uint32_t _shard_id, const std::string &hostname, const std::string &port = "443");
        virtual ~WSClient();
        virtual void write(const std::string &data);
        virtual bool HandleBuffer(std::string &buffer);
        virtual void close();
};

