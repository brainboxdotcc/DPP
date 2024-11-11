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
#include <sys/event.h>
#include <sys/sysctl.h>
#include <unistd.h>

#if defined __NetBSD__ && __NetBSD_Version__ <= 999001400
	#define CAST_TYPE intptr_t
#else
	#define CAST_TYPE void*
#endif

namespace dpp {

struct socket_engine_kqueue : public socket_engine_base {

	int kqueue_handle{INVALID_SOCKET};
	unsigned int change_pos = 0;
	std::vector<struct kevent> change_list;
	std::vector<struct kevent> ke_list;

	socket_engine_kqueue(const socket_engine_kqueue&) = default;
	socket_engine_kqueue(socket_engine_kqueue&&) = default;
	socket_engine_kqueue& operator=(const socket_engine_kqueue&) = default;
	socket_engine_kqueue& operator=(socket_engine_kqueue&&) = default;

	socket_engine_kqueue() : kqueue_handle(kqueue()) {
		change_list.resize(8);
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

	struct kevent* get_change_kevent()
	{
		if (change_pos >= change_list.size()) {
			change_list.resize(change_list.size() * 2);
		}
		return &change_list[change_pos++];
	}

	void process_events() final {
		struct timespec ts{};
		ts.tv_sec = 1;

		int i = kevent(kqueue_handle, &change_list.front(), change_pos, &ke_list.front(), static_cast<int>(ke_list.size()), &ts);
		change_pos = 0;

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
			if (kev.flags & EV_EOF) {
				eh->on_error(kev.ident, *eh, kev.fflags);
				continue;
			}
			if (filter == EVFILT_WRITE) {
				const int bits_to_clr = WANT_WRITE;
				eh->flags &= ~bits_to_clr;
				eh->on_write(kev.ident, *eh);
			}
			else if (filter == EVFILT_READ) {
				eh->on_read(kev.ident, *eh);
			}
		}
	}

	void set_event_write_flags(dpp::socket fd, socket_events* eh, uint8_t old_mask, uint8_t new_mask)
	{
		if (((new_mask & WANT_WRITE) != 0) && ((old_mask & WANT_WRITE) == 0))
		{
			struct kevent* ke = get_change_kevent();
			EV_SET(ke, fd, EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, static_cast<CAST_TYPE>(eh));
		}
		else if (((old_mask & WANT_WRITE) != 0) && ((new_mask & WANT_WRITE) == 0))
		{
			struct kevent* ke = get_change_kevent();
			EV_SET(ke, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
		}
	}

	bool register_socket(const socket_events& e) final {
		bool r = socket_engine_base::register_socket(e);
		if (r) {
			struct kevent* ke = get_change_kevent();
			socket_events* se = fds.find(e.fd)->second.get();
			if ((se->flags & WANT_READ) != 0) {
				EV_SET(ke, e.fd, EVFILT_READ, EV_ADD, 0, 0, static_cast<CAST_TYPE>(se));
			}
			set_event_write_flags(e.fd, se, 0, e.flags);
			if (fds.size() * 2 > ke_list.size()) {
				ke_list.resize(fds.size() * 2);
			}
		}
		return r;
	}

	bool update_socket(const socket_events& e) final {
		bool r = socket_engine_base::update_socket(e);
		if (r) {
			struct kevent* ke = get_change_kevent();
			socket_events* se = fds.find(e.fd)->second.get();
			if ((se->flags & WANT_READ) != 0) {
				EV_SET(ke, e.fd, EVFILT_READ, EV_ADD, 0, 0, static_cast<CAST_TYPE>(se));
			}
			set_event_write_flags(e.fd, se, 0, e.flags);
			if (fds.size() * 2 > ke_list.size()) {
				ke_list.resize(fds.size() * 2);
			}
		}
		return r;
	}

protected:

	bool remove_socket(dpp::socket fd) final {
		bool r = socket_engine_base::remove_socket(fd);
		if (r) {
			struct kevent* ke = get_change_kevent();
			EV_SET(ke, fd, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);

			// Then remove the read filter.
			ke = get_change_kevent();
			EV_SET(ke, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
		}
		return r;
	}
};

std::unique_ptr<socket_engine_base> create_socket_engine() {
	return std::make_unique<socket_engine_kqueue>();
}

};

