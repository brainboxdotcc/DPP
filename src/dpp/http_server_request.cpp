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
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstdlib>
#include <climits>
#include <dpp/http_server_request.h>
#include <dpp/utility.h>
#include <dpp/cluster.h>

namespace dpp {

constexpr std::array verb {
	"GET",
	"POST",
	"PUT",
	"PATCH",
	"DELETE",
	"HEAD",
	"CONNECT",
	"OPTIONS",
	"TRACE",
};

http_server_request::http_server_request(cluster* creator, socket fd, uint16_t port, bool plaintext_downgrade, const std::string& private_key, const std::string& public_key, http_server_request_event handle_request)
	: ssl_connection(creator, fd, port, plaintext_downgrade, private_key, public_key),
	  timeout(time(nullptr) + 10),
	  handler(handle_request),
	  state(HTTPS_HEADERS),
	  timed_out(false)
{
	http_server_request::connect();
}

void http_server_request::connect()
{
	state = HTTPS_HEADERS;
	read_loop();
}

const std::string http_server_request::get_header(const std::string& header_name) const {
	auto hdrs = request_headers.find(lowercase(header_name));
	if (hdrs != request_headers.end()) {
		return hdrs->second;
	}
	return std::string();
}

size_t http_server_request::get_header_count(const std::string& header_name) const {
	return request_headers.count(lowercase(header_name));
}

std::list<std::string> http_server_request::get_header_list(const std::string& header_name) const {
	auto hdrs = request_headers.equal_range(lowercase(header_name));
	if (hdrs.first != request_headers.end()) {
		std::list<std::string> data;
		for ( auto i = hdrs.first; i != hdrs.second; ++i ) {
			data.emplace_back(i->second);
		}
		return data;
	}
	return std::list<std::string>();
}

std::multimap<std::string, std::string> http_server_request::get_headers() const {
	return request_headers;
}

uint64_t http_server_request::get_max_post_size() const {
	return 16384;
}

uint64_t http_server_request::get_max_header_size() const {
	return 8192;
}

void http_server_request::generate_error(uint16_t error_code, const std::string& message) {
	status = HTTPS_DONE;
	owner->queue_work(1, [this, error_code, message]() {
		status = error_code;
		response_body = message;
		handler(this);
		socket_write(get_response());
		handler = {};
	});
}

bool http_server_request::handle_buffer(std::string &buffer)
{
	bool state_changed = false;
	do {
		state_changed = false;
		switch (state) {
			case HTTPS_HEADERS:
				if (buffer.length() > get_max_header_size()) {
					owner->log(ll_warning, "HTTTP request exceeds max header size, dropped");
					return false;
				} else if (buffer.find("\r\n\r\n") != std::string::npos) {

					/* Add 10 seconds to retrieve body */
					timeout += 10;

					/* Got all headers, proceed to new state */

					/* Get headers string */
					std::string headers = buffer.substr(0, buffer.find("\r\n\r\n"));

					/* Modify buffer, remove headers section */
					buffer.erase(0, buffer.find("\r\n\r\n") + 4);

					/* Process headers into map */
					std::vector<std::string> h = utility::tokenize(headers);

					if (h.empty()) {
						generate_error(400, "Malformed request");
						return true;
					}

					/* First line is special */
					std::vector<std::string> verb_path_protocol = utility::tokenize(h[0], " ");
					if (verb_path_protocol.size() < 3) {
						generate_error(400, "Malformed request");
						return true;
					}
					std::string req_verb = uppercase(verb_path_protocol[0]);
					std::string req_path = verb_path_protocol[1];
					std::string protocol = uppercase(verb_path_protocol[2]);

					h.erase(h.begin());

					if (protocol.substr(0, 5) != "HTTP/") {
						generate_error(400, "Malformed request");
						return true;
					}

					if (std::find(verb.begin(), verb.end(), req_verb) == verb.end()) {
						generate_error(401, "Unsupported method");
						return true;
					}

					for(auto &hd : h) {
						std::string::size_type sep = hd.find(": ");
						if (sep != std::string::npos) {
							std::string key = hd.substr(0, sep);
							std::string value = hd.substr(sep + 2, hd.length());
							request_headers.emplace(lowercase(key), value);
						}
					}
					auto it_cl = request_headers.find("content-length");
					if ( it_cl != request_headers.end()) {
						content_length = std::stoull(it_cl->second);
						if (content_length > get_max_post_size()) {
							content_length = get_max_post_size();
						}
					}
					state = HTTPS_CONTENT;
					state_changed = true;
					continue;
				}
			break;
			case HTTPS_CONTENT:
				request_body += buffer;
				buffer.clear();
				if (request_body.length() > get_max_post_size() || content_length == ULLONG_MAX || request_body.length() >= content_length) {
					state = HTTPS_DONE;
					state_changed = true;
				}
			break;
			case HTTPS_DONE:
				if (handler) {
					owner->queue_work(1, [this]() {
						handler(this);
						socket_write(get_response());
						handler = {};
					});
				}
				return true;
			default:
				return false;
		}
	} while (state_changed);
	return true;
}

void http_server_request::on_buffer_drained() {
	if (state == HTTPS_DONE && status > 0 && !handler) {
		this->close();
	}
}


http_server_request& http_server_request::set_status(uint16_t new_status) {
	status = new_status;
	return *this;
}

http_server_request& http_server_request::set_response_body(const std::string& new_content) {
	response_body = new_content;
	return *this;
}

std::string http_server_request::get_response_body() const {
	return response_body;
}

std::string http_server_request::get_request_body() const {
	return request_body;
}

http_server_request& http_server_request::set_response_header(const std::string& header, const std::string& value) {
	response_headers.emplace(header, value);
	return *this;
}

http_state http_server_request::get_state() const {
	return this->state;
}

uint16_t http_server_request::get_status() const {
	return this->status;
}

void http_server_request::one_second_timer() {
	if (!tcp_connect_done && time(nullptr) >= timeout) {
		timed_out = true;
		this->close();
	} else if (tcp_connect_done && !connected && time(nullptr) >= timeout && this->state != HTTPS_DONE) {
		this->close();
		timed_out = true;
	} else if (time(nullptr) >= timeout && this->state != HTTPS_DONE) {
		this->close();
		timed_out = true;
	}
}

std::string http_server_request::get_response() {
	std::string response = "HTTP/1.0 " + std::to_string(status) + " OK\r\n";
	set_response_header("Content-Length", std::to_string(response_body.length()));
	for (const auto& header : response_headers) {
		response += header.first + ": " + header.second + "\r\n";
	}
	response += "\r\n";
	response += response_body;
	return response;
}

void http_server_request::close() {
	state = HTTPS_DONE;
	ssl_connection::close();
}

http_server_request::~http_server_request() {
	if (sfd != INVALID_SOCKET) {
		ssl_connection::close();
	}
}

}
