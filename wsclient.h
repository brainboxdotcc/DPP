#pragma once
#include <string>
#include <map>
#include <vector>
#include "sslclient.h"

enum WSState {
	HTTP_HEADERS,
	CONNECTED
};

class WSClient : public SSLClient
{
	std::string key;
	WSState state;
	std::map<std::string, std::string> HTTPHeaders;
	bool parseheader(std::string &buffer);
	bool unpack(std::string &buffer);
public:
        WSClient(const std::string &hostname, const std::string &port = "443");
        virtual ~WSClient();
        virtual void write(const std::string &data);
        virtual bool HandleBuffer(std::string &buffer);
        virtual void close();
};

