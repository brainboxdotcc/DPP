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
#include <vector>
#ifdef _WIN32
/* Windows-specific sockets includes */
	#include <WinSock2.h>
	#include <WS2tcpip.h>
	#include <io.h>
	/* Windows doesn't have standard poll(), it has WSAPoll.
	 * It's the same thing with different symbol names.
	 * Microsoft gotta be different.
	 */
	#define poll(fds, nfds, timeout) WSAPoll(fds, nfds, timeout)
	#define pollfd WSAPOLLFD
	/* Windows sockets library */
	#pragma comment(lib, "ws2_32")
#else
/* Anything other than Windows (e.g. sane OSes) */
	#include <poll.h>
	#include <sys/socket.h>
#endif
#include <memory>

struct socket_engine_poll : public socket_engine_base {

	/* We store the pollfds as a vector. This means that insertion, deletion and updating
	 * are comparatively slow O(n), but these operations don't happen too often. Obtaining the
	 * list to pass to poll, which can happen several times a second, is as simple as
	 * calling poll_set.data() and is O(1). We don't expect mind-blowing performance with poll()
	 * anyway.
	 */
	std::vector<pollfd> poll_set;

	void process_events() final {
		const int poll_delay = 1000;
		int i = poll(poll_set.data(), static_cast<unsigned int>(poll_set.size()), poll_delay);
		int processed = 0;

		for (size_t index = 0; index < poll_set.size() && processed < i; index++) {
			const int fd = poll_set[index].fd;
			const short revents = poll_set[index].revents;

			if (revents > 0) {
				processed++;
			}

			auto iter = fds.find(fd);
			if (iter == fds.end()) {
				continue;
			}
			socket_events* eh = iter->second.get();

			if ((revents & POLLHUP) != 0) {
				eh->on_error(fd, *eh, 0);
				continue;
			}

			if ((revents & POLLERR) != 0) {
				socklen_t codesize = sizeof(int);
				int errcode{};
				if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &errcode, &codesize) < 0) {
					errcode = errno;
				}
				eh->on_error(fd, *eh, errcode);
				continue;
			}

			if ((revents & POLLIN) != 0) {
				eh->on_read(fd, *eh);
			}

			if ((revents & POLLOUT) != 0) {
				int mask = eh->flags;
				mask &= ~WANT_WRITE;
				eh->flags = mask;
				eh->on_write(fd, *eh);
			}
		}
	}

	bool register_socket(dpp::socket fd, const socket_events& e) final {
		bool r = socket_engine_base::register_socket(fd, e);
		if (r) {
			pollfd fd_info{};
			fd_info.fd = fd;
			fd_info.events = 0;
			if ((e.flags & WANT_READ) != 0) {
				fd_info.events |= POLLIN;
			}
			if ((e.flags & WANT_WRITE) != 0) {
				fd_info.events |= POLLOUT;
			}
			if ((e.flags & WANT_ERROR) != 0) {
				fd_info.events |= POLLERR;
			}
			poll_set.push_back(fd_info);
		}
		return r;
	}

	bool update_socket(dpp::socket fd, const socket_events& e) final {
		bool r = socket_engine_base::update_socket(fd, e);
		if (r) {
			/* We know this will succeed */
			for (pollfd& fd_info : poll_set) {
				if (fd_info.fd != fd) {
					continue;
				}
				fd_info.events = 0;
				if ((e.flags & WANT_READ) != 0) {
					fd_info.events |= POLLIN;
				}
				if ((e.flags & WANT_WRITE) != 0) {
					fd_info.events |= POLLOUT;
				}
				if ((e.flags & WANT_ERROR) != 0) {
					fd_info.events |= POLLERR;
				}
				break;
			}
		}
		return r;
	}

	bool remove_socket(dpp::socket fd) final {
		bool r = socket_engine_base::remove_socket(fd);
		if (r) {
			for (auto i = poll_set.begin(); i != poll_set.end(); ++i) {
				if (i->fd == fd) {
					poll_set.erase(i);
					return true;
				}
			}
		}
		return false;
	}
};

std::unique_ptr<socket_engine_base> create_socket_engine() {
	return std::make_unique<socket_engine_poll>();
}
