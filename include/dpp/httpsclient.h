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
#pragma once
#include <dpp/export.h>
#include <string>
#include <map>
#include <vector>
#include <variant>
#include <dpp/sslclient.h>

namespace dpp {


/**
 * @brief HTTP connection status
 */
enum http_state : uint8_t {
	/**
	 * @brief Sending/receiving HTTP headers and request body
	 */
	HTTPS_HEADERS,

	/**
	 * @brief Receiving body content.
	 */
	HTTPS_CONTENT,

	/**
	 * @brief Completed connection, as it was closed or the body is >= Content-Length
	 * 
	 */
	HTTPS_DONE,
};

typedef std::multimap<std::string, std::string> http_headers;

struct multipart_content {
	std::string body;
	std::string mimetype;
};

/**
 * @brief Implements a HTTPS socket client based on the SSL client
 */
class DPP_EXPORT https_client : public ssl_client
{
	/** Current HTTPS state */
	http_state state;

	std::string request_type;

	/** Path part of URL for HTTPS connection */
	std::string path;

	std::string request_body;

	/** Received HTTP body */
	std::string body;

	uint64_t content_length;

	http_headers request_headers;

	uint16_t status;

	/** HTTP headers received on connecting/upgrading */
	std::map<std::string, std::string> response_headers;

	/** Parse headers for a websocket frame from the buffer.
	 * @param buffer The buffer to operate on. Will modify the string removing completed items from the head of the queue
	 */
	bool parseheader(std::string &buffer);

protected:

	/** Connect */
	virtual void connect();

	/** Get websocket state
	 * @return websocket state
	 */
	http_state get_state();

public:

	/**
	 * @brief Connect to a specific HTTP server.
	 * 
	 * @param hostname 
	 * @param port 
	 * @param urlpath 
	 * @param verb 
	 * @param req_body 
	 * @param extra_headers 
	 * @param plaintext_connection
	 */
        https_client(const std::string &hostname, uint16_t port = 443, const std::string &urlpath = "/", const std::string &verb = "GET", const std::string &req_body = "", const http_headers& extra_headers = {}, bool plaintext_connection = false);

	/** Destructor */
        virtual ~https_client();

	static multipart_content build_multipart(const std::string &json, const std::vector<std::string>& filenames = {}, const std::vector<std::string>& contents = {});

	/**
	 * @brief Processes incoming data from the SSL socket input buffer.
	 * 
	 * @param buffer The buffer contents. Can modify this value removing the head elements when processed.
	 */
        virtual bool handle_buffer(std::string &buffer);

	/**
	 * @brief Close HTTPS socket
	 */
        virtual void close();

	/** Fires every second from the underlying socket I/O loop, used for sending websocket pings */
	virtual void one_second_timer();

	std::string get_header(std::string header_name);

	std::string get_content();

	uint16_t get_status();

};

};