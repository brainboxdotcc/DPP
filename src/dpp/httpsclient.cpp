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
#include <algorithm>
#include <stdlib.h>
#include <climits>
#include <dpp/httpsclient.h>
#include <dpp/utility.h>
#include <dpp/fmt-minimal.h>
#include <dpp/exception.h>

namespace dpp {

https_client::https_client(const std::string &hostname, uint16_t port,  const std::string &urlpath, const std::string &verb, const std::string &req_body, const http_headers& extra_headers, bool plaintext_connection, uint16_t request_timeout)
	: ssl_client(hostname, fmt::format("{:d}", port), plaintext_connection),
	state(HTTPS_HEADERS),
	request_type(verb),
	path(urlpath),
	request_body(req_body),
	content_length(0),
	request_headers(extra_headers),
	status(0),
	timeout(request_timeout)
{
	timeout = time(nullptr) + request_timeout;
	nonblocking = true;
	https_client::connect();
}

void https_client::connect()
{
	state = HTTPS_HEADERS;
	std::string map_headers;
	for (auto& [k,v] : request_headers) {
		map_headers += k + ": " + v + "\r\n";
	}
	this->write(
		fmt::format(

			"{} {} HTTP/1.0\r\n"
			"Host: {}\r\n"
			"pragma: no-cache\r\n"
			"Connection: close\r\n"
			"Content-Length: {}\r\n{}"
			"\r\n{}",

			this->request_type, this->path, this->hostname,
			this->request_body.length(), map_headers,
			this->request_body
		)
	);
	read_loop();
}

multipart_content https_client::build_multipart(const std::string &json, const std::vector<std::string>& filenames, const std::vector<std::string>& contents) {
	if (filenames.empty() && contents.empty()) {
		return { json, "application/json" };
	} else {
		const std::string two_cr("\r\n\r\n");
		const std::string boundary(fmt::format("-------------{:8x}{:16x}", time(nullptr) + time(nullptr), time(nullptr) * time(nullptr)));
		const std::string mime_part_start("--" + boundary + "\r\nContent-Type: application/octet-stream\r\nContent-Disposition: form-data; ");
		
		std::string content("--" + boundary);

		/* Special case, single file */
		content += "\r\nContent-Type: application/json\r\nContent-Disposition: form-data; name=\"payload_json\"" + two_cr;
		content += json + "\r\n";
		if (filenames.size() == 1 && contents.size() == 1) {
			content += mime_part_start + "name=\"file\"; filename=\"" + filenames[0] + "\"" + two_cr;
			content += contents[0];
		} else {
			/* Multiple files */
			for (size_t i = 0; i < filenames.size(); ++i) {
				content += mime_part_start + "name=\"files[" + fmt::format("{:d}", i) + "]\"; filename=\"" + filenames[i] + "\"" + two_cr;
				content += contents[i];
				content += "\r\n";
			}
		}
		content += "\r\n--" + boundary + "--";
		return { content, "multipart/form-data; boundary=" + boundary };
	}
}

const std::string https_client::get_header(std::string header_name) const {
	std::transform(header_name.begin(), header_name.end(), header_name.begin(), [](unsigned char c){
		return std::tolower(c);
	});
	auto hdrs = response_headers.find(header_name);
	if (hdrs != response_headers.end()) {
		return hdrs->second;
	}
	return std::string();
}

const std::map<std::string, std::string> https_client::get_headers() const {
	return response_headers;
}

https_client::~https_client()
{
}

bool https_client::handle_buffer(std::string &buffer)
{
	switch (state) {
		case HTTPS_HEADERS:
			if (buffer.find("\r\n\r\n") != std::string::npos) {
				/* Got all headers, proceed to new state */

				/* Get headers string */
				std::string headers = buffer.substr(0, buffer.find("\r\n\r\n"));

				/* Modify buffer, remove headers section */
				buffer.erase(0, buffer.find("\r\n\r\n") + 4);

				/* Process headers into map */
				std::vector<std::string> h = utility::tokenize(headers);
				if (h.size()) {
					std::string status_line = h[0];
					h.erase(h.begin());
					/* HTTP/1.1 200 OK */
					std::vector<std::string> req_status = utility::tokenize(status_line, " ");
					if (req_status.size() >= 3 && (req_status[0] == "HTTP/1.1" || req_status[0] == "HTTP/1.0") && atoi(req_status[1].c_str()) >= 100) {
						for(auto &hd : h) {
							std::string::size_type sep = hd.find(": ");
							if (sep != std::string::npos) {
								std::string key = hd.substr(0, sep);
								std::string value = hd.substr(sep + 2, hd.length());
								std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c){
									return std::tolower(c);
								});
								response_headers[key] = value;
							}
						}
						if (response_headers.find("content-length") != response_headers.end()) {
							content_length = std::stoull(response_headers["content-length"]);
						} else {
							content_length = ULLONG_MAX;
						}
						state = HTTPS_CONTENT;
						status = atoi(req_status[1].c_str());
					} else {
						/* Non-HTTP-like response with invalid headers. Go no further. */
						return false;
					}
				}

				if (buffer.size()) {
					body += buffer;
					buffer.clear();
					if (body.length() >= content_length) {
						state = HTTPS_DONE;
					}
				}

			}
		break;
		case HTTPS_CONTENT:
			body += buffer;
			buffer.clear();
			if (body.length() >= content_length) {
				state = HTTPS_DONE;
			}
		break;
		case HTTPS_DONE:
			this->close();
			return false;
		break;
	}
	return true;
}

uint16_t https_client::get_status() const {
	return status;
}

const std::string https_client::get_content() const {
	return body;
}

http_state https_client::get_state() {
	return this->state;
}

void https_client::one_second_timer() {
	if (time(nullptr) >= timeout && this->state != HTTPS_DONE) {
		this->close();
	}
}

void https_client::close() {
	if (state != HTTPS_DONE) {
		state = HTTPS_DONE;
		ssl_client::close();
	}
}

http_connect_info https_client::get_host_info(std::string url) {
	http_connect_info hci = { false, "http", "", 80};
	if (url.substr(0, 8) == "https://") {
		hci.port = 443;
		hci.is_ssl = true;
		hci.scheme = url.substr(0, 5);
		url = url.substr(8, url.length());
	} else if (url.substr(0, 7) == "http://") {
		hci.scheme = url.substr(0, 4);
		url = url.substr(7, url.length());
	} else if (url.substr(0, 11) == "discord.com") {
		hci.scheme = "https";
		hci.is_ssl = true;
		hci.port = 443;
	}
	size_t colon_pos = url.find(':');
	if (colon_pos != std::string::npos) {
		hci.hostname = url.substr(0, colon_pos);
		hci.port = atoi(url.substr(colon_pos + 1, url.length()).c_str());
		if (hci.port == 0) {
			hci.port = 80;
		}
	} else {
		hci.hostname = url;
	}
	return hci;
}

};