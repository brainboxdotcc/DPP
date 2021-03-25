#include <string>
#include <iostream>
#include "wsclient.h"

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
			std::cout << "Received in connected state: " << buffer << std::endl;
		break;
	}
	return true;
}

void WSClient::close()
{
	SSLClient::close();
}

int main(int argc, char const *argv[])
{
	//Host is hardcoded to localhost for testing purposes
	WSClient client("gateway.discord.gg");
	client.ReadLoop();
	client.close();
	return 0;
}

