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
#include <dpp/httpsclient.h>
#include <dpp/utility.h>
#include <dpp/fmt/format.h>

namespace dpp {

https_client::https_client(const std::string &hostname, uint16_t port,  const std::string &urlpath, const std::string &verb, const std::string &req_body, const http_headers& extra_headers)
	: ssl_client(hostname, fmt::format("{:d}", port)),
	state(HTTPS_HEADERS),
	request_type(verb),
	path(urlpath),
	request_body(req_body),
	content_length(0),
	request_headers(extra_headers)
{
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

multipart_response https_client::build_multipart(const std::string &json, const std::vector<std::string>& filenames, const std::vector<std::string>& contents) {
	if (filenames.empty() && contents.empty()) {
		return { json, "application/json" };
	} else {
		std::string boundary(fmt::format("-------------BOUNDARY_{:8x}_YRADNUOB_{:16x}", time(nullptr) + time(nullptr), time(nullptr) * time(nullptr)));
		std::string content("--" + boundary);
		/* Special case, single file */
		content += "\r\nContent-Type: application/json\r\nContent-Disposition: form-data; name=\"payload_json\"\r\n\r\n";
		content += json + "\r\n";
		if (filenames.size() == 1 && contents.size() == 1) {
			content += "--" + boundary + "\r\nContent-Type: application/octet-stream\r\nContent-Disposition: form-data; name=\"file\"; filename=\"" + filenames[0] + "\"\r\n\r\n";
			content += contents[0];
		} else {
			/* Multiple files */
			for (size_t i = 0; i < filenames.size(); ++i) {
				content += "--" + boundary + "\r\nContent-Type: application/octet-stream\r\nContent-Disposition: form-data; name=\"files[" + fmt::format("{:d}", i) + "]\"; filename=\"" + filenames[i] + "\"\r\n\r\n";
				content += contents[i];
				content += "\r\n";
			}
		}
		content += "\r\n--" + boundary + "--";
		return { content, "multipart/form-data; boundary=" + boundary + "" };
	}
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
					std::vector<std::string> status = utility::tokenize(status_line, " ");
					if (status.size() >= 3) {
						for(auto &hd : h) {
							std::string::size_type sep = hd.find(": ");
							if (sep != std::string::npos) {
								std::string key = hd.substr(0, sep);
								std::string value = hd.substr(sep + 2, hd.length());
								std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c){
									return std::tolower(c);
								});
								response_headers[key] = value;
								std::cout << key << ": " << value << "\n";
							}
						}
						if (response_headers.find("content-length") != response_headers.end()) {
							content_length = std::stoull(response_headers["content-length"]);
						}
						state = HTTPS_CONTENT;
						std::cout << "r=" << status[1] << "\n" << status_line << "\n";
					}
				}
			}
		break;
		case HTTPS_CONTENT:
			body += buffer;
			buffer.clear();
			if (body.length() >= content_length) {
				std::cout << "blen=" << body.length() << "\n";
				std::cout << "b=" << body << "\n";
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

http_state https_client::get_state()
{
	return this->state;
}

void https_client::one_second_timer()
{
}

void https_client::close()
{
	if (state != HTTPS_DONE) {
		state = HTTPS_DONE;
		ssl_client::close();
	}
}

};