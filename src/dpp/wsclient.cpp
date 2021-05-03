/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * Copyright 2021 Craig Edwards and D++ contributors 
 * (https://github.com/brainboxdotcc/DPP/graphs/contributors)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ************************************************************************************/
#include <string>
#include <iostream>
#include <fstream>
#include <dpp/wsclient.h>
#include <fmt/format.h>

namespace dpp {

const unsigned char WS_MASKBIT = (1 << 7);
const unsigned char WS_FINBIT = (1 << 7);
const unsigned char WS_PAYLOAD_LENGTH_MAGIC_LARGE = 126;
const unsigned char WS_PAYLOAD_LENGTH_MAGIC_HUGE = 127;
const size_t WS_MAX_PAYLOAD_LENGTH_SMALL = 125;
const size_t WS_MAX_PAYLOAD_LENGTH_LARGE = 65535;
const size_t MAXHEADERSIZE = sizeof(uint64_t) + 2;

WSClient::WSClient(const std::string &hostname, const std::string &port, const std::string &urlpath)
	: SSLClient(hostname, port),
	state(HTTP_HEADERS),
	key(fmt::format("{:16x}", time(nullptr))),
	path(urlpath)
{
}

void WSClient::Connect()
{
	state = HTTP_HEADERS;
	/* Send headers synchronously */
	this->write(
		fmt::format(

			"GET {} HTTP/1.1\r\n"
			"Host: {}\r\n"
			"pragma: no-cache\r\n"
			"User-Agent: DPP/0.1\r\n"
			"Upgrade: WebSocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Key: {}\r\n"
			"Sec-WebSocket-Version: 13\r\n\r\n",

			this->path, this->hostname, this->key
		)
	);
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
					} else {
						return false;
					}
				}
			}
		break;
		case CONNECTED:
			/* Process packets until we can't */
			while (this->parseheader(buffer));
		break;
	}
	return true;
}

WSState WSClient::GetState()
{
	return this->state;
}

bool WSClient::parseheader(std::string &data)
{
	if (data.size() < 4) {
		/* Not enough data to form a frame yet */
		return false;
	} else {
		unsigned char opcode = data[0];
		switch (opcode & ~WS_FINBIT)
		{
			case OP_CONTINUATION:
			case OP_TEXT:
			case OP_BINARY:
			case OP_PING:
			case OP_PONG:
			{
				std::string payload;

				unsigned char len1 = data[1];
				unsigned int payloadstartoffset = 2;

				if (len1 & WS_MASKBIT) {
					len1 &= ~WS_MASKBIT;
					payloadstartoffset += 2;
					/* We don't handle masked data, because discord doesnt send it */
					return true;
				}

				/* 6 bit ("small") length frame */
				uint64_t len = len1;

				if (len1 == WS_PAYLOAD_LENGTH_MAGIC_LARGE) {
					/* 24 bit ("large") length frame */
					if (data.length() < 8) {
						/* We don't have a complete header yet */
						return false;
					}

					unsigned char len2 = (unsigned char)data[2];
					unsigned char len3 = (unsigned char)data[3];
					len = (len2 << 8) | len3;

					payloadstartoffset += 2;
				} else if (len1 == WS_PAYLOAD_LENGTH_MAGIC_HUGE) {
					/* 64 bit ("huge") length frame */
					if (data.length() < 10) {
						/* We don't have a complete header yet */
						return false;
					}
					len = 0;
					for (int v = 2, shift = 56; v < 10; ++v, shift -= 8) {
						unsigned char l = (unsigned char)data[v];
						len |= (uint64_t)(l & 0xff) << shift;
					}
					payloadstartoffset += 8;
				}

				if (data.length() < payloadstartoffset + len) {
					/* We don't have a complete frame yet */
					return false;
				}

				/* Copy from buffer into string */
				const std::string::iterator endit = data.begin() + payloadstartoffset + len;
				for (std::string::const_iterator i = data.begin() + payloadstartoffset; i != endit; ++i) {
					const unsigned char c = (unsigned char)*i;
					payload.push_back(c);
				}
		
				/* Remove this frame from the input buffer */
				data.erase(data.begin(), endit);

				if ((opcode & ~WS_FINBIT) == OP_PING || (opcode & ~WS_FINBIT) == OP_PONG) {
					HandlePingPong((opcode & ~WS_FINBIT) == OP_PING, payload);
				} else {
					/* Pass this frame to the deriving class */
					this->HandleFrame(payload);
				}
				return true;
			}
			break;

			case OP_CLOSE:
			{
				uint16_t error = data[2] & 0xff;
			       	error <<= 8;
				error |= (data[3] & 0xff);
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
	return false;
}

void WSClient::OneSecondTimer()
{
	if (((time(NULL) % 20) == 0) && (state == CONNECTED)) {
		/* For sending pings, we send with payload */
		unsigned char out[MAXHEADERSIZE];
		std::string payload = "keepalive";
		size_t s = this->FillHeader(out, payload.length(), OP_PING);
		std::string header((const char*)out, s);
		SSLClient::write(header);
		SSLClient::write(payload);
	}
}

void WSClient::HandlePingPong(bool ping, const std::string &payload)
{
	unsigned char out[MAXHEADERSIZE];
	if (ping) {
		/* For receving pings we echo back their payload with the type OP_PONG */
		size_t s = this->FillHeader(out, payload.length(), OP_PONG);
		std::string header((const char*)out, s);
		SSLClient::write(header);
		SSLClient::write(payload);
	}
}

void WSClient::Error(uint32_t errorcode)
{
}

void WSClient::close()
{
	this->state = HTTP_HEADERS;
	SSLClient::close();
}

};