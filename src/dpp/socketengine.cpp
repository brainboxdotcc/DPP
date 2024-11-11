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

#include <dpp/socketengine.h>
#include <dpp/exception.h>
#include <csignal>
#include <memory>
#include <sslclient.h>

namespace dpp {

bool socket_engine_base::register_socket(const socket_events &e) {
	if (e.fd > INVALID_SOCKET && fds.find(e.fd) == fds.end()) {
		fds.emplace(e.fd, std::make_unique<socket_events>(e));
		return true;
	}
	return false;
}

bool socket_engine_base::update_socket(const socket_events &e) {
	if (e.fd > INVALID_SOCKET && fds.find(e.fd) != fds.end()) {
		auto iter = fds.find(e.fd);
		*(iter->second) = e;
		return true;
	}
	return false;
}

bool socket_engine_base::remove_socket(dpp::socket fd) {
	auto iter = fds.find(fd);
	if (iter != fds.end()) {
		fds.erase(iter);
		return true;
	}
	return false;
}

socket_engine_base::socket_engine_base() {
#ifndef WIN32
	set_signal_handler(SIGALRM);
	set_signal_handler(SIGHUP);
	set_signal_handler(SIGPIPE);
	set_signal_handler(SIGCHLD);
	set_signal_handler(SIGXFSZ);
#else
	// Set up winsock.
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		throw dpp::connection_exception(err_connect_failure, "WSAStartup failure");
	}
#endif
	pool = std::make_unique<thread_pool>();
}

}
