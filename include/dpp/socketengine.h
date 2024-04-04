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

namespace dpp {

enum socket_event_flags : uint8_t {
	WANT_READ = 1,
	WANT_WRITE = 2,
	WANT_ERROR = 4,
};

using socket_read_event = auto (*)(dpp::socket fd, const struct socket_events&) -> void;
using socket_write_event = auto (*)(dpp::socket fd, const struct socket_events&) -> void;
using socket_error_event = auto (*)(dpp::socket fd, const struct socket_events&, int error_code) -> void;

struct socket_events {
	uint8_t flags{0};
	socket_read_event on_read{};
	socket_write_event on_write{};
	socket_error_event on_error{};
};

using socket_container = std::unordered_map<dpp::socket, std::unique_ptr<socket_events>>;

struct socket_engine_base {
	socket_container fds;

	socket_engine_base();
	socket_engine_base(const socket_engine_base&) = default;
	socket_engine_base(socket_engine_base&&) = default;
	socket_engine_base& operator=(const socket_engine_base&) = default;
	socket_engine_base& operator=(socket_engine_base&&) = default;

	virtual ~socket_engine_base() = default;

	virtual void process_events() = 0;
	virtual bool register_socket(dpp::socket fd, const socket_events& e);
	virtual bool update_socket(dpp::socket fd, const socket_events& e);
	virtual bool remove_socket(dpp::socket fd);
};

/* This is implemented by whatever derived form socket_engine takes */
std::unique_ptr<socket_engine_base> create_socket_engine();

};
