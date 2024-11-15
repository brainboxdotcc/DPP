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

#include <dpp/dpp.h>
#include <iostream>
#include <thread>
#include <unistd.h>
#include <dpp/dns.h>
#include <dpp/sslclient.h>

int main() {
	dpp::cluster cl("no-token");
	auto se = dpp::create_socket_engine(&cl);

	const dpp::dns_cache_entry* addr = dpp::resolve_hostname("neuron.brainbox.cc", "80");
	std::cout << "Connect to IP: " << addr->resolved_addr << "\n";
	dpp::socket sfd = addr->make_connecting_socket();
	dpp::address_t destination = addr->get_connecting_address(80);
	if (sfd == INVALID_SOCKET) {
		std::cerr << "Couldn't create outbound socket on port 80\n";
		exit(1);
	} else if (::connect(sfd, destination.get_socket_address(), destination.size()) != 0) {
		dpp::close_socket(sfd);
		std::cerr << "Couldn't connect outbound socket on port 80\n";
		exit(1);
	}

	dpp::socket_events events(
		sfd,
		dpp::WANT_READ | dpp::WANT_WRITE | dpp::WANT_ERROR,
		[&se](dpp::socket fd, const struct dpp::socket_events& e) {
			int r = 0;
			do {
				char buf[128]{0};
				r = ::read(e.fd, buf, 127);
				if (r > 0) {
					buf[127] = 0;
					std::cout << buf;
					std::cout.flush();
				}
			} while (r > 0);
			if (r == 0 || (errno && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)) {
				dpp::close_socket(fd);
				se->delete_socket(fd);
			}
		},
		[](dpp::socket fd, const struct dpp::socket_events& e) {
			std::cout << "WANT_WRITE event on socket " << fd << "\n";
			::write(e.fd, "GET / HTTP/1.0\r\nConnection: close\r\n\r\n", strlen("GET / HTTP/1.0\r\nConnection: close\r\n\r\n"));
		},
		[](dpp::socket fd, const struct dpp::socket_events&, int error_code) {
		  std::cout << "WANT_ERROR event on socket " << fd << " with code " << error_code << "\n";
		}
	);

	se->register_socket(events);

	do {
		se->process_events();
	} while (true);
}