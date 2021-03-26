#include <string>
#include <iostream>
#include <fstream>
#include <dpp/wsclient.h>

const unsigned char WS_MASKBIT = (1 << 7);
const unsigned char WS_FINBIT = (1 << 7);
const unsigned char WS_PAYLOAD_LENGTH_MAGIC_LARGE = 126;
const unsigned char WS_PAYLOAD_LENGTH_MAGIC_HUGE = 127;
const size_t WS_MAX_PAYLOAD_LENGTH_SMALL = 125;
const size_t WS_MAX_PAYLOAD_LENGTH_LARGE = 65535;
const size_t MAXHEADERSIZE = sizeof(uint64_t) + 2;

WSClient::WSClient(const std::string &hostname, const std::string &port) : SSLClient(hostname, port), state(HTTP_HEADERS), key("DASFcazvbgest")
{
	/* Send headers synchronously */
	this->write("GET /?v=6&encoding=json HTTP/1.1\r\n" 
			"Host: " + hostname + "\r\n"
			"pragma: no-cache\r\n"
			"Upgrade: WebSocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Key: " + key + "\r\n"
			"Sec-WebSocket-Version: 13\r\n\r\n");
}

WSClient::~WSClient()
{
}

bool WSClient::HandleFrame(const std::string &buffer)
{
	/* This is a stub for classes that derive the websocket client */
	return true;
}

size_t WSClient::FillHeader(unsigned char* outbuf, size_t sendlength, OpCode opcode)
{
	size_t pos = 0;
	outbuf[pos++] = WS_FINBIT | opcode;

	if (sendlength <= WS_MAX_PAYLOAD_LENGTH_SMALL)
	{
		outbuf[pos++] = sendlength;
	}
	else if (sendlength <= WS_MAX_PAYLOAD_LENGTH_LARGE)
	{
		outbuf[pos++] = WS_PAYLOAD_LENGTH_MAGIC_LARGE;
		outbuf[pos++] = (sendlength >> 8) & 0xff;
		outbuf[pos++] = sendlength & 0xff;
	}
	else
	{
		outbuf[pos++] = WS_PAYLOAD_LENGTH_MAGIC_HUGE;
		const uint64_t len = sendlength;
		for (int i = sizeof(uint64_t)-1; i >= 0; i--)
			outbuf[pos++] = ((len >> i*8) & 0xff);
	}

	/* Masking - We don't care about masking, but discord insists on it. We send a mask of 0x00000000 because
	 * any value XOR 0 is itself, meaning we dont have to waste time and effort on this crap.
	 */
	outbuf[1] |= WS_MASKBIT;
	outbuf[pos++] = 0;
	outbuf[pos++] = 0;
	outbuf[pos++] = 0;
	outbuf[pos++] = 0;

	return pos;
}


void WSClient::write(const std::string &data)
{
	if (state == HTTP_HEADERS) {
		/* Simple write */
		SSLClient::write(data);
	} else {
		unsigned char out[MAXHEADERSIZE];
		size_t s = this->FillHeader(out, data.length(), OP_TEXT);
		std::string header((const char*)out, s);
		SSLClient::write(header);
		SSLClient::write(data);
	}
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
		
						state = CONNECTED;
						//std::cout << "Websocket connected\n";
					} else {
						//std::cout << "Unexpected status: " << status_line << std::endl;
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

bool WSClient::parseheader(std::string &buffer)
{
	if (buffer.size() < 4) {
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
					/* We don't handle masked data, because discord doesnt send it */
					return false;
				}

				uint64_t len = len1;

				if (len1 == WS_PAYLOAD_LENGTH_MAGIC_LARGE) {
					if (buffer.length() < 8) {
						/* We don't have a complete header yet */
						return true;
					}

					unsigned char len2 = (unsigned char)buffer[2];
					unsigned char len3 = (unsigned char)buffer[3];
					len = (len2 << 8) | len3;

					payloadstartoffset += 2;
				} else if (len1 == WS_PAYLOAD_LENGTH_MAGIC_HUGE) {
					if (buffer.length() < 10) {
						/* We don't have a complete header yet */
						return true;
					}

					unsigned char len2 = (unsigned char)buffer[2];
					unsigned char len3 = (unsigned char)buffer[3];
					unsigned char len4 = (unsigned char)buffer[4];
					unsigned char len5 = (unsigned char)buffer[5];
					unsigned char len6 = (unsigned char)buffer[6];
					unsigned char len7 = (unsigned char)buffer[7];
					unsigned char len8 = (unsigned char)buffer[8];
					unsigned char len9 = (unsigned char)buffer[9];

					payloadstartoffset += 8;
					len = 0;
					len |= (uint64_t)(len2 & 0xff) << 52;
					len |= (uint64_t)(len3 & 0xff) << 48;
					len |= (uint64_t)(len4 & 0xff) << 40;
					len |= (uint64_t)(len5 & 0xff) << 32;
					len |= (uint64_t)(len6 & 0xff) << 24;
					len |= (uint64_t)(len7 & 0xff) << 16;
					len |= (uint64_t)(len8 & 0xff) << 8;
					len |= (uint64_t)(len9 & 0xff) << 0;

				}

				if (buffer.length() < payloadstartoffset + len) {
					/* We don't have a complete frame yet */
					return true;
				}

				/* Copy from buffer into string */
				const std::string::iterator endit = buffer.begin() + payloadstartoffset + len;
				for (std::string::const_iterator i = buffer.begin() + payloadstartoffset; i != endit; ++i) {
					const unsigned char c = (unsigned char)*i;
					erl.push_back(c);
				}
		
				/* Remove this frame from the input buffer */
				buffer.erase(buffer.begin(), endit);

				/* Pass this frame to the deriving class */
				return this->HandleFrame(erl);
			}
			break;

			case OP_PING:
			{
			}
			break;

			case OP_PONG:
			{
			}
			break;

			case OP_CLOSE:
			{
				uint16_t error = buffer[2] & 0xff;
			       	error <<= 8;
				error |= (buffer[3] & 0xff);
				this->Error(error);
				return false;
			}
			break;

			default:
			{
				this->Error(0);
				return false;
			}
			break;
		}
	}
	return true;
}

void WSClient::Error(uint32_t errorcode)
{
}

void WSClient::close()
{
	SSLClient::close();
}

