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
#include <array>
#include <cstdint>
#include <sys/types.h>
#include <unistd.h>
#include <dpp/cluster.h>
#include "kqueue-facade.h"

namespace dpp {

struct DPP_EXPORT socket_engine_kqueue : public socket_engine_base {

	static constexpr size_t MAX_SOCKET_VALUE = 65536;

	int kqueue_handle{INVALID_SOCKET};
	std::array<struct kevent, MAX_SOCKET_VALUE> ke_list;

	socket_engine_kqueue(const socket_engine_kqueue&) = delete;
	socket_engine_kqueue(socket_engine_kqueue&&) = delete;
	socket_engine_kqueue& operator=(const socket_engine_kqueue&) = delete;
	socket_engine_kqueue& operator=(socket_engine_kqueue&&) = delete;

	explicit socket_engine_kqueue(cluster* creator) : socket_engine_base(creator), kqueue_handle(kqueue()) {
		if (kqueue_handle == -1) {
			throw dpp::connection_exception("Failed to initialise kqueue()");
		}
		stats.engine_type = "kqueue";
	}

	~socket_engine_kqueue() override {
		if (kqueue_handle != INVALID_SOCKET) {
			close(kqueue_handle);
		}
	}

	void process_events() final {
		struct timespec ts{};
		ts.tv_sec = 1;

		int i = kevent(kqueue_handle, nullptr, 0, ke_list.data(), static_cast<int>(ke_list.size()), &ts);
		if (i < 0) {
			prune();
			return;
		}

		for (int j = 0; j < i; j++) {
			const struct kevent& kev = ke_list[j];
			auto eh = get_fd(kev.ident);
			if (eh == nullptr) {
				continue;
			}

			if ((eh->flags & WANT_DELETION) == 0L) try {

				const short filter = kev.filter;
				if (kev.flags & EV_EOF || kev.flags & EV_ERROR) {
					if (eh->on_error) {
						eh->on_error(kev.ident, *eh, kev.fflags);
						stats.errors++;
					}
					continue;
				}
				if (filter == EVFILT_WRITE) {
					const int bits_to_clr = WANT_WRITE;
					eh->flags &= ~bits_to_clr;
					if (eh->on_write) {
						eh->on_write(kev.ident, *eh);
						stats.writes++;
					}
				}
				else if (filter == EVFILT_READ) {
					if (eh->on_read) {
						eh->on_read(kev.ident, *eh);
						stats.reads++;
					}
				}

			} catch (const std::exception& e) {
				stats.errors++;
				owner->log(ll_trace, "Socket loop exception: " + std::string(e.what()));
				eh->on_error(kev.ident, *eh, 0);
			}


			if ((eh->flags & WANT_DELETION) != 0L) {
				remove_socket(kev.ident);
				fds.erase(kev.ident);
			}
		}
		prune();
	}

	bool set_events(const socket_events& e) {
		struct kevent ke{};
		if ((e.flags & WANT_READ) != 0) {
			EV_SET(&ke, e.fd, EVFILT_READ, EV_ADD, 0, 0, nullptr);
			kevent(kqueue_handle, &ke, 1, nullptr, 0, nullptr);
		}
		if ((e.flags & WANT_WRITE) != 0) {
			EV_SET(&ke, e.fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, nullptr);
			kevent(kqueue_handle, &ke, 1, nullptr, 0, nullptr);
		}
		return true;
	}

	bool register_socket(const socket_events& e) final {
		if (socket_engine_base::register_socket(e)) {
			return set_events(e);
		}
		return false;
	}

	bool update_socket(const socket_events& e) final {
		if (socket_engine_base::update_socket(e)) {
			return set_events(e);
		}
		return false;
	}

protected:

	bool remove_socket(dpp::socket fd) final {
		struct kevent ke{};
		EV_SET(&ke, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
		kevent(kqueue_handle, &ke, 1, nullptr, 0, nullptr);
		EV_SET(&ke, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
		kevent(kqueue_handle, &ke, 1, nullptr, 0, nullptr);
		if (!owner->on_socket_close.empty()) {
			socket_close_t event(owner, 0, "");
			event.fd = fd;
			owner->on_socket_close.call(event);
		}
		return true;
	}
};

DPP_EXPORT std::unique_ptr<socket_engine_base> create_socket_engine(cluster *creator) {
	return std::make_unique<socket_engine_kqueue>(creator);
}

};

