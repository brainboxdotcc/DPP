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
#include <dpp/exception.h>
#include <dpp/socketengine.h>
#include <memory>
#include <windows.h>
#include <winsock2.h>

namespace dpp {

struct DPP_EXPORT socket_engine_win32 : public socket_engine_base {

	static constexpr UINT WM_SOCKET = WM_USER + 1;
	static constexpr wchar_t WINDOW_CLASS[] = L"DPP_SOCKET_ENGINE";

	HWND window{nullptr};

	socket_engine_win32(const socket_engine_win32 &) = delete;
	socket_engine_win32(socket_engine_win32 &&) = delete;
	socket_engine_win32 &operator=(const socket_engine_win32 &) = delete;
	socket_engine_win32 &operator=(socket_engine_win32 &&) = delete;

	explicit socket_engine_win32(cluster *creator) : socket_engine_base(creator) {
		stats.engine_type = "win32";
	}

	void process_events() final {
		/*
		 * The message-only window must be created by the thread which runs the
		 * socket engine. A cluster may use st_wait, where this is the thread
		 * which calls cluster::start(), or st_return, where this is the cluster
		 * event thread.
		 *
		 * The cluster and socket engine may therefore have been constructed on
		 * a different thread from the one which calls process_events(). The
		 * window must NOT be created in the socket engine constructor.
		 *
		 * process_events() is expected to continue to run on the same thread
		 * for the lifetime of the socket engine.
		 */
		if (window == nullptr) {
			initialise();
		}

		MSG msg{};
		DWORD result = MsgWaitForMultipleObjects(0, nullptr, FALSE, 1000, QS_ALLINPUT);

		if (result != WAIT_FAILED) {
			while (PeekMessageW(&msg, window, WM_SOCKET, WM_SOCKET, PM_REMOVE)) {
				process_socket_event(static_cast<dpp::socket>(msg.wParam), msg.lParam);
			}
		}

		prune();
	}

	bool register_socket(const socket_events &e) final {
		if (!socket_engine_base::register_socket(e)) {
			return false;
		}

		/*
		 * Sockets may be registered before cluster::start() enters the event
		 * loop. initialise() registers these when the engine thread starts.
		 */
		if (window == nullptr) {
			return true;
		}

		if (WSAAsyncSelect(e.fd, window, WM_SOCKET, get_events(e)) == SOCKET_ERROR) {
			std::lock_guard<std::shared_mutex> lock(fds_mutex);
			fds.erase(e.fd);
			return false;
		}

		return true;
	}

	bool update_socket(const socket_events &e) final {
		if (!socket_engine_base::update_socket(e)) {
			return false;
		}

		if (window == nullptr) {
			return true;
		}

		return WSAAsyncSelect(e.fd, window, WM_SOCKET, get_events(e)) != SOCKET_ERROR;
	}

protected:
	bool remove_socket(dpp::socket fd) final {
		if (window != nullptr) {
			WSAAsyncSelect(fd, window, 0, 0);
		}

		if (!owner->on_socket_close.empty()) {
			socket_close_t event(owner, 0, "");
			event.fd = fd;
			owner->on_socket_close.call(event);
		}

		return true;
	}

private:
	static long get_events(const socket_events &e) {
		long events = FD_CLOSE;

		if ((e.flags & WANT_READ) != 0) {
			events |= FD_READ;
		}
		if ((e.flags & WANT_WRITE) != 0) {
			events |= FD_WRITE;
		}

		return events;
	}

	static void register_window_class() {
		static const bool registered = []() {
			WNDCLASSW wc{};
			wc.lpfnWndProc = DefWindowProcW;
			wc.hInstance = GetModuleHandleW(nullptr);
			wc.lpszClassName = WINDOW_CLASS;

			if (RegisterClassW(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
				throw dpp::connection_exception("Failed to register Windows socket engine window class");
			}

			return true;
		}();

		(void) registered;
	}

	void initialise() {
		register_window_class();

		HINSTANCE instance = GetModuleHandleW(nullptr);
		window = CreateWindowExW(0, WINDOW_CLASS, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, instance, nullptr);
		if (window == nullptr) {
			throw dpp::connection_exception("Failed to create Windows socket engine message window");
		}

		/*
		 * register_socket() may have been called before the event loop started.
		 * Register all sockets already known to the base engine now that the
		 * message window exists on the correct thread.
		 */
		std::shared_lock lock(fds_mutex);
		for (const auto &[fd, events]: fds) {
			if (WSAAsyncSelect(fd, window, WM_SOCKET, get_events(*events)) == SOCKET_ERROR) {
				throw dpp::connection_exception("Failed to register socket with Windows socket engine: " + std::to_string(WSAGetLastError()));
			}
		}
	}

	void process_socket_event(dpp::socket fd, LPARAM lparam) {
		socket_events *eh = get_fd(fd);
		if (eh == nullptr || (eh->flags & WANT_DELETION) != 0L) {
			return;
		}

		const int network_event = WSAGETSELECTEVENT(lparam);
		const int error = WSAGETSELECTERROR(lparam);

		try {
			if (error != 0) {
				stats.errors++;
				if (eh->on_error) {
					eh->on_error(fd, *eh, error);
				}
			} else if (network_event == FD_CLOSE) {
				stats.errors++;
				if (eh->on_error) {
					eh->on_error(fd, *eh, 0);
				}
			} else if (network_event == FD_WRITE) {
				/*
				 * WANT_WRITE is one-shot in the D++ socket engine interface,
				 * matching the epoll and kqueue implementations.
				 */
				eh->flags &= ~WANT_WRITE;
				update_socket(*eh);

				if (eh->on_write) {
					stats.writes++;
					eh->on_write(fd, *eh);
				}
			} else if (network_event == FD_READ) {
				if (eh->on_read) {
					stats.reads++;
					eh->on_read(fd, *eh);
				}
			}
		} catch (const std::exception &e) {
			stats.errors++;
			owner->log(ll_trace, "Socket loop exception: " + std::string(e.what()));
			if (eh->on_error) {
				eh->on_error(fd, *eh, 0);
			}
		}

		if ((eh->flags & WANT_DELETION) != 0L) {
			remove_socket(fd);
			std::lock_guard<std::shared_mutex> lock(fds_mutex);
			fds.erase(fd);
		}
	}
};

DPP_EXPORT std::unique_ptr<socket_engine_base> create_socket_engine(cluster *creator) {
	return std::make_unique<socket_engine_win32>(creator);
}

}