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
#include <memory>
#include <sys/epoll.h>

struct socket_engine_epoll : public socket_engine_base {

	int epoll_handle{INVALID_SOCKET};
	const int epoll_hint = 128;

	socket_engine_epoll() : epoll_handle(epoll_create(epoll_hint)) {

		if (epoll_handle == -1) {
			throw dpp::connection_exception("Failed to initialise epoll()");
		}
	}

	~socket_engine_epoll() {
		if (epoll_handle != INVALID_SOCKET) {
			close(epoll_handle);
		}
	}

	void run() override {
		// TODO: event routing loop for epoll() goes here
	}

	bool register_socket(dpp::socket fd, const socket_events& e) final {
		bool r = socket_engine_base::register_socket(fd, e);
		if (r) {
			struct epoll_event ev{};
			if ((e.flags & WANT_READ) != 0) {
				ev.events |= EPOLLIN;
			}
			if ((e.flags & WANT_WRITE) != 0) {
				ev.events |= EPOLLOUT;
			}
			ev.data.ptr = fds.find(fd)->second.get();
			int i = epoll_ctl(epoll_handle, EPOLL_CTL_ADD, fd, &ev);
			if (i < 0) {
				throw dpp::connection_exception("Failed to register socket to epoll_ctl()");
			}
		}
		return r;
	}

	bool update_socket(dpp::socket fd, const socket_events& e) final {
		bool r = socket_engine_base::update_socket(fd, e);
		if (r) {
			struct epoll_event ev{};
			if ((e.flags & WANT_READ) != 0) {
				ev.events |= EPOLLIN;
			}
			if ((e.flags & WANT_WRITE) != 0) {
				ev.events |= EPOLLOUT;
			}
			ev.data.ptr = fds.find(fd)->second.get();
			int i = epoll_ctl(epoll_handle, EPOLL_CTL_MOD, fd, &ev);
			if (i < 0) {
				throw dpp::connection_exception("Failed to modify socket with epoll_ctl()");
			}
		}
		return r;
	}

	bool remove_socket(dpp::socket fd) final {
		bool r = socket_engine_base::remove_socket(fd);
		if (r) {
			struct epoll_event ev{};
			int i = epoll_ctl(epoll_handle, EPOLL_CTL_DEL, fd, &ev);
			if (i < 0) {
				throw dpp::connection_exception("Failed to deregister socket with epoll_ctl()");
			}

		}
		return r;
	}
};

std::unique_ptr<socket_engine_base> create_socket_engine() {
	return std::make_unique<socket_engine_epoll>();
}