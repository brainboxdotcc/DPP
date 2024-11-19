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
#include <vector>
#include <sys/types.h>
#include <unistd.h>
#include <dpp/cluster.h>
#include "kqueue-facade.h"

namespace dpp {

struct socket_engine_kqueue : public socket_engine_base {

	int kqueue_handle{INVALID_SOCKET};
	std::vector<struct kevent> ke_list;

	socket_engine_kqueue(const socket_engine_kqueue&) = delete;
	socket_engine_kqueue(socket_engine_kqueue&&) = delete;
	socket_engine_kqueue& operator=(const socket_engine_kqueue&) = delete;
	socket_engine_kqueue& operator=(socket_engine_kqueue&&) = delete;

	explicit socket_engine_kqueue(cluster* creator) : socket_engine_base(creator), kqueue_handle(kqueue()) {
		ke_list.resize(16);
		if (kqueue_handle == -1) {
			throw dpp::connection_exception("Failed to initialise kqueue()");
		}
	}

	~socket_engine_kqueue() override {
		if (kqueue_handle != INVALID_SOCKET) {
			close(kqueue_handle);
		}
	}

	void process_events() final {
		struct timespec ts{};
		ts.tv_sec = 1;

		int i = kevent(kqueue_handle, NULL, 0, &ke_list.front(), static_cast<int>(ke_list.size()), &ts);
		if (i < 0) {
			return;
		}

		for (int j = 0; j < i; j++) {
			const struct kevent& kev = ke_list[j];
			auto* eh = reinterpret_cast<socket_events*>(kev.udata);
			if (eh == nullptr) {
				continue;
			}

			const short filter = kev.filter;
			if (kev.flags & EV_EOF || kev.flags & EV_ERROR) {
				if (eh->on_error) {
					eh->on_error(kev.ident, *eh, kev.fflags);
				}
				continue;
			}
			if (filter == EVFILT_WRITE) {
				const int bits_to_clr = WANT_WRITE;
				eh->flags &= ~bits_to_clr;
				if (eh->on_write) {
					eh->on_write(kev.ident, *eh);
				}
			}
			else if (filter == EVFILT_READ) {
				if (eh->on_read) {
					eh->on_read(kev.ident, *eh);
				}
			}
		}
	}

	bool register_socket(const socket_events& e) final {
		bool r = socket_engine_base::register_socket(e);
		if (r) {
			struct kevent ke;
			socket_events* se = fds.find(e.fd)->second.get();
			if ((se->flags & WANT_READ) != 0) {
				EV_SET(&ke, e.fd, EVFILT_READ, EV_ADD, 0, 0, static_cast<CAST_TYPE>(se));
				kevent(kqueue_handle, &ke, 1, 0, 0, nullptr);
			}
			if ((se->flags & WANT_WRITE) != 0) {
				EV_SET(&ke, e.fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, static_cast<CAST_TYPE>(se));
				kevent(kqueue_handle, &ke, 1, 0, 0, nullptr);
			}
			if (fds.size() * 3 > ke_list.size()) {
				ke_list.resize(fds.size() * 3);
			}
		}
		return r;
	}

	bool update_socket(const socket_events& e) final {
		bool r = socket_engine_base::update_socket(e);
		if (r) {
			socket_events* se = fds.find(e.fd)->second.get();
			struct kevent ke;
			if ((e.flags & WANT_READ) != 0) {
				EV_SET(&ke, e.fd, EVFILT_READ, EV_ADD, 0, 0, static_cast<CAST_TYPE>(se));
				kevent(kqueue_handle, &ke, 1, 0, 0, nullptr);
			}
			if ((e.flags & WANT_WRITE) != 0) {
				EV_SET(&ke, e.fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, static_cast<CAST_TYPE>(se));
				kevent(kqueue_handle, &ke, 1, 0, 0, nullptr);
			}
			if (fds.size() * 3 > ke_list.size()) {
				ke_list.resize(fds.size() * 3);
			}
		}
		return r;
	}

protected:

	bool remove_socket(dpp::socket fd) final {
		bool r = socket_engine_base::remove_socket(fd);
		if (r) {
			struct kevent ke;
			EV_SET(&ke, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
			kevent(kqueue_handle, &ke, 1, 0, 0, nullptr);
			EV_SET(&ke, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
			kevent(kqueue_handle, &ke, 1, 0, 0, nullptr);
		}
		return r;
	}
};

std::unique_ptr<socket_engine_base> create_socket_engine(cluster* creator) {
	return std::make_unique<socket_engine_kqueue>(creator);
}

};

