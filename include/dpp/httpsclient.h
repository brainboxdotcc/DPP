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

struct http_connect_info {
	bool is_ssl;
	std::string scheme;
	std::string hostname;
	uint16_t port;
};

/**
 * @brief Implements a HTTPS socket client based on the SSL client
 */
class DPP_EXPORT https_client : public ssl_client
{
	/**
	 * @brief Current connection state
	 */
	http_state state;

	/**
	 * @brief The type of the request, e.g. GET, POST
	 */
	std::string request_type;

	/**
	 * @brief Path part of URL for HTTPS connection
	 */
	std::string path;

	/**
	 * @brief The request body, e.g. form data
	 */
	std::string request_body;

	/**
	 * @brief The response body, e.g. file content or JSON
	 */
	std::string body;

	/**
	 * @brief The reported length of the content. If this is
	 * UULONG_MAX, then no length was reported by the server.
	 */
	uint64_t content_length;

	/**
	 * @brief Headers for the request, e.g. Authorization, etc.
	 */
	http_headers request_headers;

	/**
	 * @brief The status of the HTTP request from the server,
	 * e.g. 200 for OK, 404 for not found. A value of 0 means
	 * no request has been completed.
	 */
	uint16_t status;

	/**
	 * @brief Time at which the request should be abandoned
	 */
	time_t timeout;

	/**
	 * @brief Headers from the server's response, e.g. RateLimit
	 * headers, cookies, etc.
	 */
	std::map<std::string, std::string> response_headers;

protected:

	/**
	 * @brief Start the connection
	 */
	virtual void connect();

	/**
	 * @brief Get request state
	 * @return request state
	 */
	http_state get_state();

public:

	/**
	 * @brief Connect to a specific HTTP(S) server and complete a request.
	 * 
	 * The constructor will attempt the connection, and return the content.
	 * By the time the constructor completes, the HTTP request will be stored
	 * in the object. This is a blocking call.
	 * 
	 * @param hostname 
	 * @param port 
	 * @param urlpath 
	 * @param verb 
	 * @param req_body 
	 * @param extra_headers 
	 * @param plaintext_connection
	 * @param request_timeout
	 */
        https_client(const std::string &hostname, uint16_t port = 443, const std::string &urlpath = "/", const std::string &verb = "GET", const std::string &req_body = "", const http_headers& extra_headers = {}, bool plaintext_connection = false, uint16_t request_timeout = 5);

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

	const std::string get_header(std::string header_name) const;

	const std::map<std::string, std::string> get_headers() const;
 
	const std::string get_content() const;

	uint16_t get_status() const;

	static http_connect_info get_host_info(std::string url);

};

};