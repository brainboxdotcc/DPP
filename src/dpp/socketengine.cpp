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
#include <csignal>
#include <memory>

bool socket_engine_base::register_socket(dpp::socket fd, const socket_events &e) {
	if (fd > INVALID_SOCKET && fds.find(fd) == fds.end()) {
		fds.emplace(fd, std::make_unique<socket_events>(e));
		return true;
	}
	return false;
}

bool socket_engine_base::update_socket(dpp::socket fd, const socket_events &e) {
	if (fd > INVALID_SOCKET && fds.find(fd) != fds.end()) {
		auto iter = fds.find(fd);
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
		signal(SIGALRM, SIG_IGN);
		signal(SIGHUP, SIG_IGN);
		signal(SIGPIPE, SIG_IGN);
		signal(SIGCHLD, SIG_IGN);
		signal(SIGXFSZ, SIG_IGN);
	#else
		// Set up winsock.
		WSADATA wsadata;
		if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {
			throw dpp::connection_exception(err_connect_failure, "WSAStartup failure");
		}
	#endif
}
