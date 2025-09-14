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

#include <dpp/cluster.h>
#include <dpp/compat.h>
#include <dpp/socketengine.h>
#include <dpp/exception.h>
#include <dpp/socket.h>
#include <dpp/sslconnection.h>
#include <vector>
#include <shared_mutex>
#include <memory>
#include <cerrno>

namespace dpp {

struct DPP_EXPORT socket_engine_poll : public socket_engine_base {

	/* We store the pollfds as a vector. This means that insertion, deletion and updating
	 * are comparatively slow O(n), but these operations don't happen too often. Obtaining the
	 * list to pass to poll, which can happen several times a second, is as simple as
	 * calling poll_set.data() and is O(1). We don't expect mind-blowing performance with poll()
	 * anyway.
	 */
	std::vector<pollfd> poll_set;
	pollfd out_set[FD_SETSIZE]{0};
	std::shared_mutex poll_set_mutex;

	void process_events() final {
		const int poll_delay = 1000;

		prune();
		/* Save count of tracked sockets while mutex is held, just in case */
		size_t fd_count = 0;
		{
			std::shared_lock lock(poll_set_mutex);
			if (poll_set.empty()) {
				/* On many platforms, it is not possible to wait on an empty set */
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				return;
			} else {
				if (poll_set.size() > FD_SETSIZE) {
					throw dpp::connection_exception("poll() does not support more than FD_SETSIZE active sockets at once!");
				}
				fd_count = poll_set.size();
				/**
				 * We must make a copy of the poll_set, because it would cause thread locking/contention
				 * issues if we had it locked for read during poll/iteration of the returned set.
				 */
				std::copy(poll_set.begin(), poll_set.end(), out_set);
			}
		}

		int i = dpp::compat::poll(out_set, static_cast<unsigned int>(fd_count), poll_delay);
		int processed = 0;

		for (size_t index = 0; index < fd_count && processed < i; index++) {
			const dpp::socket fd = out_set[index].fd;
			const short revents = out_set[index].revents;

			if (revents > 0) {
				processed++;
			}

			if (fd == wake_read.fd) {
				if ((revents & POLLIN) != 0) {
					drain_wakeup();
				}
				continue;
			}

			socket_events *eh = get_fd(fd);
			if (eh == nullptr) {
				continue;
			}

			if ((eh->flags & WANT_DELETION) == 0L) try {

				if ((revents & POLLHUP) != 0) {
					eh->on_error(fd, *eh, 0);
					stats.errors++;
					continue;
				}

				if ((revents & POLLERR) != 0) {
					socklen_t codesize = sizeof(int);
					int errcode{};
					if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *) &errcode, &codesize) < 0) {
						errcode = errno;
					}
					stats.errors++;
					eh->on_error(fd, *eh, errcode);
					continue;
				}

				if ((revents & POLLIN) != 0) {
					stats.reads++;
					eh->on_read(fd, *eh);
				}

				if ((revents & POLLOUT) != 0) {
					stats.writes++;
					eh->flags &= ~WANT_WRITE;
					update_socket(*eh);
					eh->on_write(fd, *eh);
				}

			} catch (const std::exception &e) {
				stats.errors++;
				eh->on_error(fd, *eh, 0);
			}

			if ((eh->flags & WANT_DELETION) != 0L) {
				remove_socket(fd);
				std::unique_lock lock(fds_mutex);
				fds.erase(fd);
			}
		}
	}

	bool register_socket(const socket_events& e) final {
		bool r = socket_engine_base::register_socket(e);
		if (r) {
			std::unique_lock lock(poll_set_mutex);
			pollfd fd_info{};
			fd_info.fd = e.fd;
			fd_info.events = 0;
			if ((e.flags & WANT_READ) != 0) {
				fd_info.events |= POLLIN;
			}
			if ((e.flags & WANT_WRITE) != 0) {
				fd_info.events |= POLLOUT;
			}
			poll_set.push_back(fd_info);
		}
		refresh_poll();
		return r;
	}

	bool update_socket(const socket_events& e) final {
		bool r = socket_engine_base::update_socket(e);
		if (r) {
			std::unique_lock lock(poll_set_mutex);
			/* We know this will succeed */
			for (pollfd& fd_info : poll_set) {
				if (fd_info.fd != e.fd) {
					continue;
				}
				fd_info.events = 0;
				if ((e.flags & WANT_READ) != 0) {
					fd_info.events |= POLLIN;
				}
				if ((e.flags & WANT_WRITE) != 0) {
					fd_info.events |= POLLOUT;
				}
				break;
			}
		}
		refresh_poll();
		return r;
	}

	explicit socket_engine_poll(cluster* creator) : socket_engine_base(creator) {
		stats.engine_type = "poll";
		init_wakeup_socket();
	};

protected:

	/* Poll wakeup mechanism: UDP loopback socket pair */
	dpp::raii_socket wake_read{dpp::rst_udp};
	dpp::raii_socket wake_write{dpp::rst_udp};

	bool remove_socket(dpp::socket fd) final {
		std::unique_lock lock(poll_set_mutex);
		for (auto i = poll_set.begin(); i != poll_set.end(); ++i) {
			if (i->fd == fd) {
				poll_set.erase(i);
				if (!owner->on_socket_close.empty()) {
					socket_close_t event(owner, 0, "");
					event.fd = fd;
					owner->on_socket_close.call(event);
				}
				refresh_poll();
				return true;
			}
		}
		return false;
	}

	void init_wakeup_socket() {
		if (!wake_read.bind(dpp::address_t("127.0.0.1", 0))) {
			throw dpp::connection_exception("Failed to bind refresh_poll read socket");
		}

		if (!set_nonblocking(wake_read.fd, true)) {
			throw dpp::connection_exception("Failed to set refresh_poll read socket non-blocking");
		}

		dpp::address_t tmp;
		uint16_t port = tmp.get_port(wake_read.fd);
		dpp::address_t dest("127.0.0.1", port);
		if (::connect(wake_write.fd, dest.get_socket_address(), static_cast<int>(dest.size())) != 0) {
			throw dpp::connection_exception("Failed to connect refresh_poll write socket");
		}

		{
			std::unique_lock lock(poll_set_mutex);
			pollfd fd_info{};
			fd_info.fd = wake_read.fd;
			fd_info.events = POLLIN;
			poll_set.push_back(fd_info);
		}
	}

	void drain_wakeup() {
		char buf[256];
		while (true) {
#if _WIN32
			int r = ::recv(wake_read.fd, buf, sizeof(buf), 0);
			if (r <= 0) {
				int e = WSAGetLastError();
				if (e == WSAEWOULDBLOCK || e == WSAEINTR) {
					break;
				}
				break;
			}
#else
			ssize_t r = ::recv(wake_read.fd, buf, sizeof(buf), MSG_DONTWAIT);
			if (r < 0) {
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					break;
				}
				break;
			}
			if (r == 0) {
				break;
			}
#endif
		}
	}

	void refresh_poll() const {
		if (wake_write.fd == INVALID_SOCKET) return;
		static const char one = 1;
		(void)::send(wake_write.fd, &one, 1, 0);
	}
};

DPP_EXPORT std::unique_ptr<socket_engine_base> create_socket_engine(cluster* creator) {
	return std::make_unique<socket_engine_poll>(creator);
}

};

