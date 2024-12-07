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
#include <unistd.h>
#include <vector>
#include <iostream>

namespace dpp {

int modify_event(int epoll_handle, socket_events* eh, int new_events) {
	if (new_events != eh->flags) {
		struct epoll_event new_ev{};
		new_ev.events = EPOLLET;
		if ((new_events & WANT_READ) != 0) {
			new_ev.events |= EPOLLIN;
		}
		if ((new_events & WANT_WRITE) != 0) {
			new_ev.events |= EPOLLOUT;
		}
		if ((new_events & WANT_ERROR) != 0) {
			new_ev.events |= EPOLLERR;
		}
		new_ev.data.ptr = static_cast<void *>(eh);
		epoll_ctl(epoll_handle, EPOLL_CTL_MOD, eh->fd, &new_ev);
	}
	return new_events;
}

struct DPP_EXPORT socket_engine_epoll : public socket_engine_base {

	int epoll_handle{INVALID_SOCKET};
	static constexpr size_t MAX_EVENTS = 65536;
	std::array<struct epoll_event, MAX_EVENTS> events;

	socket_engine_epoll(const socket_engine_epoll&) = delete;
	socket_engine_epoll(socket_engine_epoll&&) = delete;
	socket_engine_epoll& operator=(const socket_engine_epoll&) = delete;
	socket_engine_epoll& operator=(socket_engine_epoll&&) = delete;

	explicit socket_engine_epoll(cluster* creator) : socket_engine_base(creator), epoll_handle(epoll_create(MAX_EVENTS)) {
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
		int i = epoll_wait(epoll_handle, events.data(), MAX_EVENTS, sleep_length);

		for (int j = 0; j < i; j++) {
			epoll_event ev = events[j];

			auto* const eh = static_cast<socket_events*>(ev.data.ptr);
			if (!eh) {
				continue;
			}

			const int fd = eh->fd;
			if (fd == INVALID_SOCKET || eh->flags & WANT_DELETION) {
				continue;
			}

			try {

				if ((ev.events & EPOLLHUP) != 0U) {
					if (eh->on_error) {
						eh->on_error(fd, *eh, EPIPE);
					}
					continue;
				}

				if ((ev.events & EPOLLERR) != 0U) {
					socklen_t codesize = sizeof(int);
					int errcode{};
					if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &errcode, &codesize) < 0) {
						errcode = errno;
					}
					if (eh->on_error) {
						eh->on_error(fd, *eh, errcode);
					}
					continue;
				}

				if ((ev.events & EPOLLOUT) != 0U) {
					/* Should we have a flag to allow keeping WANT_WRITE? Maybe like WANT_WRITE_ONCE or GREEDY_WANT_WRITE, eh */
					eh->flags = modify_event(epoll_handle, eh, eh->flags & ~WANT_WRITE);
					if (eh->on_write) {
						eh->on_write(fd, *eh);
					}
				}

				if ((ev.events & EPOLLIN) != 0U) {
					if (eh->on_read) {
						eh->on_read(fd, *eh);
					}
				}

			} catch (const std::exception& e) {
				eh->on_error(fd, *eh, 0);
			}
		}
		prune();
	}

	bool register_socket(const socket_events& e) final {
		bool r = socket_engine_base::register_socket(e);
		if (r) {
			struct epoll_event ev{};
			ev.events = EPOLLET;
			if ((e.flags & WANT_READ) != 0) {
				ev.events |= EPOLLIN;
			}
			if ((e.flags & WANT_WRITE) != 0) {
				ev.events |= EPOLLOUT;
			}
			if ((e.flags & WANT_ERROR) != 0) {
				ev.events |= EPOLLERR;
			}
			{
				std::shared_lock lock(fds_mutex);
				ev.data.ptr = fds.find(e.fd)->second.get();
			}
			return epoll_ctl(epoll_handle, EPOLL_CTL_ADD, e.fd, &ev) >= 0;
		}
		return r;
	}

	bool update_socket(const socket_events& e) final {
		bool r = socket_engine_base::update_socket(e);
		if (r) {
			struct epoll_event ev{};
			ev.events = EPOLLET;
			if ((e.flags & WANT_READ) != 0) {
				ev.events |= EPOLLIN;
			}
			if ((e.flags & WANT_WRITE) != 0) {
				ev.events |= EPOLLOUT;
			}
			if ((e.flags & WANT_ERROR) != 0) {
				ev.events |= EPOLLERR;
			}
			{
				std::shared_lock lock(fds_mutex);
				ev.data.ptr = fds.find(e.fd)->second.get();
			}
			return epoll_ctl(epoll_handle, EPOLL_CTL_MOD, e.fd, &ev) >= 0;
		}
		return r;
	}

protected:

	bool remove_socket(dpp::socket fd) final {
		bool r = socket_engine_base::remove_socket(fd);
		if (r) {
			struct epoll_event ev{};
			epoll_ctl(epoll_handle, EPOLL_CTL_DEL, fd, &ev);
		}
		return r;
	}
};

DPP_EXPORT std::unique_ptr<socket_engine_base> create_socket_engine(cluster *creator) {
	return std::make_unique<socket_engine_epoll>(creator);
}

};
