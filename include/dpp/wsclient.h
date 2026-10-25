/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * SPDX-License-Identifier: Apache-2.0
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
#pragma once
#include <dpp/export.h>
#include <string>
#include <map>
#include <dpp/sslconnection.h>

namespace dpp {

/**
 * @brief Websocket protocol types available on Discord
 */
enum websocket_protocol_t : uint8_t {
	/**
	 * @brief JSON data, text, UTF-8 character set
	 */
	ws_json = 0,

	/**
	 * @brief Erlang Term Format (ETF) binary protocol
	 */
	ws_etf = 1
};

/**
 * @brief Websocket connection status
 */
enum ws_state : uint8_t {
	/**
	 * @brief Sending/receiving HTTP headers, acting as a standard HTTP connection.
	 * This is the state prior to receiving "HTTP/1.1 101 Switching Protocols" from the
	 * server side.
	 */
	HTTP_HEADERS,

	/**
	 * @brief Connected as a websocket, and "upgraded". Now talking using binary frames.
	 */
	CONNECTED
};

/**
 * @brief Low-level websocket opcodes for frames
 */
enum ws_opcode : uint8_t {
	/**
	 * @brief Continuation.
	 */
	OP_CONTINUATION = 0x00,

	/**
	 * @brief Text frame.
	 */
	OP_TEXT = 0x01,

	/**
	 * @brief Binary frame.
	 */
	OP_BINARY = 0x02,

	/**
	 * @brief Close notification with close code.
	 */
	OP_CLOSE = 0x08,

	/**
	 * @brief Low level ping.
	 */
	OP_PING = 0x09,

	/**
	 * @brief Low level pong.
	 */
	OP_PONG = 0x0a,

	/**
	 * @brief Automatic selection of type
	 */
	 OP_AUTO = 0xff,
};

/**
 * @brief Implements a websocket client based on the SSL client
 */
class DPP_EXPORT websocket_client : public ssl_connection {
	/**
	 * @brief Connection key used in the HTTP headers
	 */
	std::string key;

	/**
	 * @brief Current websocket state
	 */
	ws_state state;

	/**
	 * @brief Path part of URL for websocket
	 */
	std::string path;

	/**
	 * @brief Data opcode, represents the type of frames we send
	 */
	ws_opcode data_opcode;

	/**
	 * @brief HTTP headers received on connecting/upgrading
	 */
	std::map<std::string, std::string> http_headers;

	/**
	 * @brief Parse headers for a websocket frame from the buffer.
	 * @param buffer The buffer to operate on. Will modify the string removing completed items from the head of the queue
	 * @return true if a complete header has been received
	 */
	bool parseheader(std::string& buffer);

	/**
	 * @brief Fill a header for outbound messages
	 * @param outbuf The raw frame to fill
	 * @param sendlength The size of the data to encapsulate
	 * @param opcode the ws_opcode to send in the header
	 * @return size of filled header
	 */
	size_t fill_header(unsigned char* outbuf, size_t sendlength, ws_opcode opcode);

	/**
	 * @brief Handle ping requests.
	 * @param payload The ping payload, to be returned as-is for a pong
	 */
	void handle_ping(const std::string& payload);

protected:

	/**
	 * @brief Connect to websocket server
	 */
	virtual void connect() override;

	/**
	 * @brief Get websocket state
	 * @return websocket state
	 */
	[[nodiscard]] ws_state get_state() const;

	/**
	 * @brief If true the connection timed out while waiting,
	 * when waiting for SSL negotiation, TCP connect(), or HTTP.
	 */
	bool timed_out;

	/**
	 * @brief Time at which the connection should be abandoned,
	 * if we are still connecting or negotiating with a HTTP server
	 */
	time_t timeout;

public:

	/**
	 * @brief Connect to a specific websocket server.
	 * @param creator Creating cluster
	 * @param hostname Hostname to connect to
	 * @param port Port to connect to
	 * @param urlpath The URL path components of the HTTP request to send
	 * @param opcode The encoding type to use, either OP_BINARY or OP_TEXT
	 * @note This just indicates the default for frames sent. Certain sockets,
	 * such as voice websockets, may send a combination of OP_TEXT and OP_BINARY
	 * frames, whereas shard websockets will only ever send OP_BINARY for ETF and
	 * OP_TEXT for JSON.
	 */
	websocket_client(cluster* creator, const std::string& hostname, const std::string& port = "443", const std::string& urlpath = "", ws_opcode opcode = OP_BINARY);

	/**
	 * @brief Destroy the websocket client object
	 */
	virtual ~websocket_client() = default;

	/**
	 * @brief Write to websocket. Encapsulates data in frames if the status is CONNECTED.
	 * @param data The data to send.
	 * @param _opcode The opcode of the data to send, either binary or text. The default
	 * is to use the socket's opcode as set in the constructor.
	 */
	virtual void write(const std::string_view data, ws_opcode _opcode = OP_AUTO);

	/**
	 * @brief Processes incoming frames from the SSL socket input buffer.
	 * @param buffer The buffer contents. Can modify this value removing the head elements when processed.
	 */
	virtual bool handle_buffer(std::string& buffer) override;

	/**
	 * @brief Close websocket
	 */
	virtual void close() override;

	/**
	 * @brief Receives raw frame content only without headers
	 *
	 * @param buffer The buffer contents
	 * @param opcode Frame type, e.g. OP_TEXT, OP_BINARY
	 * @return True if the frame was successfully handled. False if no valid frame is in the buffer.
	 */
	virtual bool handle_frame(const std::string& buffer, ws_opcode opcode);

	/**
	 * @brief Called upon error frame.
	 *
	 * @param errorcode The error code from the websocket server
	 */
	virtual void error(uint32_t errorcode);

	/**
	 * @brief Fires every second from the underlying socket I/O loop, used for sending websocket pings
	 */
	virtual void one_second_timer() override;

	/**
	 * @brief Send OP_CLOSE error code 1000 to the other side of the connection.
	 * This indicates graceful close.
	 * @note This informs Discord to invalidate the session, you cannot resume if you send this
	 */
	void send_close_packet();

	/**
	 * @brief Called on HTTP socket closure
	 */
	virtual void on_disconnect();
};

}
