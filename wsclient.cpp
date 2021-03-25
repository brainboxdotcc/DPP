#include <string>
#include <iostream>
#include "wsclient.h"

const unsigned char WS_MASKBIT = (1 << 7);
const unsigned char WS_FINBIT = (1 << 7);
const unsigned char WS_PAYLOAD_LENGTH_MAGIC_LARGE = 126;
const unsigned char WS_PAYLOAD_LENGTH_MAGIC_HUGE = 127;
const size_t WS_MAX_PAYLOAD_LENGTH_SMALL = 125;
const size_t WS_MAX_PAYLOAD_LENGTH_LARGE = 65535;
const size_t MAXHEADERSIZE = sizeof(uint64_t) + 2;

enum OpCode
{
	OP_CONTINUATION = 0x00,
	OP_TEXT = 0x01,
	OP_BINARY = 0x02,
	OP_CLOSE = 0x08,
	OP_PING = 0x09,
	OP_PONG = 0x0a
};

WSClient::WSClient(const std::string &hostname, const std::string &port) : SSLClient(hostname, port), state(HTTP_HEADERS), key("DASFcazvbgest")
{
	this->write("GET /?v=6&encoding=etf HTTP/1.1\r\nHost: "+hostname+"\r\n" +
			"pragma: no-cache\r\n" +
			"Upgrade: WebSocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Key: "+key+"\r\n"
			"Sec-WebSocket-Version: 13\r\n\r\n");
}

WSClient::~WSClient()
{
}

void WSClient::write(const std::string &data)
{
	SSLClient::write(data);
}

std::vector<std::string> tokenize(std::string const &in, const char* sep = "\r\n") {
	std::string::size_type b = 0;
	std::vector<std::string> result;

	while ((b = in.find_first_not_of(sep, b)) != std::string::npos) {
		auto e = in.find(sep, b);
		result.push_back(in.substr(b, e-b));
		b = e;
	}
	return result;
}

bool WSClient::HandleBuffer(std::string &buffer)
{
	switch (state) {
		case HTTP_HEADERS:
			if (buffer.find("\r\n\r\n") != std::string::npos) {
				/* Got all headers, proceed to new state */

				/* Get headers string */
				std::string headers = buffer.substr(0, buffer.find("\r\n\r\n"));

				/* Modify buffer, remove headers section */
				buffer.erase(0, buffer.find("\r\n\r\n") + 4);

				/* Process headers into map */
				std::vector<std::string> h = tokenize(headers);
				if (h.size()) {
					std::string status_line = h[0];
					h.erase(h.begin());
					/* HTTP/1.1 101 Switching Protocols */
					std::vector<std::string> status = tokenize(status_line, " ");
					if (status.size() >= 3 && status[1] == "101") {
						for(auto &hd : h) {
							std::string::size_type sep = hd.find(": ");
							if (sep != std::string::npos) {
								std::string key = hd.substr(0, sep);
								std::string value = hd.substr(sep + 2, hd.length());
								HTTPHeaders[key] = value;
							}
						}
		
						/* Check for valid websocket upgrade header */
						if (HTTPHeaders.find("upgrade") != HTTPHeaders.end() && HTTPHeaders["upgrade"] == "websocket") {
							state = CONNECTED;
							std::cout << "Websocket connected\n";
						} else {
							std::cout << "No upgrade header found" << std::endl;
							return false;
						}
					} else {
						std::cout << "Unexpected status: " << status_line << std::endl;
						return false;
					}
				}
			}
		break;
		case CONNECTED:
			return this->parseheader(buffer);
		break;
	}
	return true;
}

bool WSClient::unpack(std::string &buffer)
{
	uint8_t x = buffer[0];
	uint32_t y = x;
	std::cout << "ver " << y << "\n";
	x = buffer[1];
	y = x;
	std::cout << "type " << y << "\n";
	return true;
}

bool WSClient::parseheader(std::string &buffer)
{
	if (buffer.size() < 6) {
		/* Not enough data to form a frame yet */
		return true;
	} else {
		unsigned char opcode = buffer[0];
		switch (opcode & ~WS_FINBIT)
		{
			case OP_CONTINUATION:
			case OP_TEXT:
			case OP_BINARY:
			{
				std::string erl;

				unsigned char len1 = buffer[1];
				unsigned int payloadstartoffset = 2;

				if (len1 & WS_MASKBIT) {
					len1 &= ~WS_MASKBIT;
					payloadstartoffset += 2;
				}

				unsigned int len = len1;

				if (len1 == WS_PAYLOAD_LENGTH_MAGIC_LARGE) {
					if (buffer.length() < 8) {
						return true;
					}

					unsigned char len2 = (unsigned char)buffer[2];
					unsigned char len3 = (unsigned char)buffer[3];
					len = (len2 << 8) | len3;

					payloadstartoffset += 2;
				} else if (len1 == WS_PAYLOAD_LENGTH_MAGIC_HUGE) {
					/* MISSING!!! */
				}

				if (buffer.length() < payloadstartoffset + len) {
					return true;
				}

				const std::string::iterator endit = buffer.begin() + payloadstartoffset + len;
				for (std::string::const_iterator i = buffer.begin() + payloadstartoffset; i != endit; ++i) {
					const unsigned char c = (unsigned char)*i;
					erl.push_back(c);
				}
		
				buffer.erase(buffer.begin(), endit);

				return this->unpack(erl);
			}
			break;

			case OP_PING:
			{
			//	return HandlePingPongFrame(sock, true);
			}
			break;

			case OP_PONG:
			{
				// A pong frame may be sent unsolicited, so we have to handle it.
				// It may carry application data which we need to remove from the recvq as well.
			//	return HandlePingPongFrame(sock, false);
			}
			break;

			case OP_CLOSE:
			{
			//	sock->SetError("Connection closed");
				std::cout << "Connection close" << std::endl;
				return -1;
			}
			break;

			default:
			{
			//	sock->SetError("WebSocket: Invalid opcode");
				std::cout << "Invalid opcode" << std::endl;
				return -1;
			}
			break;
		}
	}
	return true;
}

void WSClient::close()
{
	SSLClient::close();
}

int main(int argc, char const *argv[])
{
	WSClient client("gateway.discord.gg");
	client.ReadLoop();
	client.close();
	return 0;
}

