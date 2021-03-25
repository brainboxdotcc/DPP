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
public:
        WSClient(uint32_t _shard_id, const std::string &hostname, const std::string &port = "443");
        virtual ~WSClient();
        virtual void write(const std::string &data);
        virtual bool HandleBuffer(std::string &buffer);
        virtual void close();
};

