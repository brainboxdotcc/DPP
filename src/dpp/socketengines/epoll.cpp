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
#include <sys/socket.h>
#include <vector>

struct socket_engine_epoll : public socket_engine_base {

	int epoll_handle{INVALID_SOCKET};
	static const int epoll_hint = 128;
	std::vector<struct epoll_event> events;

	socket_engine_epoll(const socket_engine_epoll&) = default;
	socket_engine_epoll(socket_engine_epoll&&) = default;
	socket_engine_epoll& operator=(const socket_engine_epoll&) = default;
	socket_engine_epoll& operator=(socket_engine_epoll&&) = default;

	socket_engine_epoll() : epoll_handle(epoll_create(socket_engine_epoll::epoll_hint)) {
		events.resize(socket_engine_epoll::epoll_hint);
		if (epoll_handle == -1) {
			throw dpp::connection_exception("Failed to initialise epoll()");
		}
	}

	~socket_engine_epoll() override {
		if (epoll_handle != INVALID_SOCKET) {
			close(epoll_handle);
		}
	}

	void process_events() final {
		const int sleep_length = 1000;
		int i = epoll_wait(epoll_handle, events.data(), static_cast<int>(events.size()), sleep_length);

		for (int j = 0; j < i; j++) {
			epoll_event ev = events[j];

			auto* const eh = static_cast<socket_events*>(ev.data.ptr);
			const int fd = ev.data.fd;
			if (fd == INVALID_SOCKET) {
				continue;
			}

			if ((ev.events & EPOLLHUP) != 0U) {
				eh->on_error(fd, *eh, 0);
				continue;
			}

			if ((ev.events & EPOLLERR) != 0U) {
				/* Get error number */
				socklen_t codesize = sizeof(int);
				int errcode{};
				if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &errcode, &codesize) < 0) {
					errcode = errno;
				}
				eh->on_error(fd, *eh, errcode);
				continue;
			}

			if ((ev.events & EPOLLOUT) != 0U) {
				int new_events = eh->flags & ~WANT_WRITE;
				if (new_events != eh->flags) {
					ev.events = new_events;
					ev.data.ptr = static_cast<void *>(eh);
					epoll_ctl(epoll_handle, EPOLL_CTL_MOD, fd, &ev);
				}
				eh->flags = new_events;
			}
			if (ev.events & EPOLLIN) {
				eh->on_read(fd, *eh);
			}
			if (ev.events & EPOLLOUT) {
				eh->on_write(fd, *eh);
			}
		}

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
			if ((e.flags & WANT_ERROR) != 0) {
				ev.events |= EPOLLERR;
			}
			ev.data.ptr = fds.find(fd)->second.get();
			int i = epoll_ctl(epoll_handle, EPOLL_CTL_ADD, fd, &ev);
			if (i < 0) {
				throw dpp::connection_exception("Failed to register socket to epoll_ctl()");
			}
			if (fds.size() * 2 > events.size()) {
				events.resize(fds.size() * 2);
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
			if ((e.flags & WANT_ERROR) != 0) {
				ev.events |= EPOLLERR;
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