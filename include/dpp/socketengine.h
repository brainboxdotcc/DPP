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
#pragma once
#include <dpp/export.h>
#include <dpp/socket.h>
#include <cstdint>
#include <unordered_map>
#include <memory>
#include <functional>
#include <dpp/thread_pool.h>

namespace dpp {

enum socket_event_flags : uint8_t {
	WANT_READ = 1,
	WANT_WRITE = 2,
	WANT_ERROR = 4,
};

using socket_read_event = std::function<void(dpp::socket fd, const struct socket_events&)>;
using socket_write_event = std::function<void(dpp::socket fd, const struct socket_events&)>;
using socket_error_event = std::function<void(dpp::socket fd, const struct socket_events&, int error_code)>;

struct socket_events {
	dpp::socket fd{INVALID_SOCKET};
	uint8_t flags{0};
	socket_read_event on_read{};
	socket_write_event on_write{};
	socket_error_event on_error{};
	socket_events(dpp::socket socket_fd, uint8_t _flags, const socket_read_event& read_event, const socket_write_event& write_event = {}, const socket_error_event& error_event = {})
		: fd(socket_fd), flags(_flags), on_read(read_event), on_write(write_event), on_error(error_event) { }

};

using socket_container = std::unordered_map<dpp::socket, std::unique_ptr<socket_events>>;

struct socket_engine_base {
	socket_container fds;
	std::unique_ptr<thread_pool> pool;

	socket_engine_base();
	socket_engine_base(const socket_engine_base&) = delete;
	socket_engine_base(socket_engine_base&&) = delete;
	socket_engine_base& operator=(const socket_engine_base&) = delete;
	socket_engine_base& operator=(socket_engine_base&&) = delete;

	virtual ~socket_engine_base() = default;

	virtual void process_events() = 0;
	virtual bool register_socket(const socket_events& e);
	virtual bool update_socket(const socket_events& e);
	virtual bool remove_socket(dpp::socket fd);
};

/* This is implemented by whatever derived form socket_engine takes */
std::unique_ptr<socket_engine_base> create_socket_engine();

#ifndef _WIN32
	void set_signal_handler(int signal);
#endif

};
