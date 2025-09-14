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
#include <csignal>
#include <memory>
#include <dpp/sslconnection.h>
#include <iostream>
#include <dpp/cache.h>
#include <dpp/cluster.h>

namespace dpp {

bool socket_engine_base::register_socket(const socket_events &e) {
	std::unique_lock lock(fds_mutex);
	auto i = fds.find(e.fd);
	if (e.fd != INVALID_SOCKET && i == fds.end()) {
		fds.emplace(e.fd, std::make_unique<socket_events>(e));
		stats.active_fds++;
		return true;
	}
	if (e.fd != INVALID_SOCKET && i != fds.end()) {
		remove_socket(e.fd);
		fds.erase(i);
		fds.emplace(e.fd, std::make_unique<socket_events>(e));
		stats.updates++;
		return true;
	}
	return false;
}

bool socket_engine_base::update_socket(const socket_events &e) {
	std::unique_lock lock(fds_mutex);
	if (e.fd != INVALID_SOCKET && fds.find(e.fd) != fds.end()) {
		auto iter = fds.find(e.fd);
		*(iter->second) = e;
		stats.updates++;
		return true;
	}
	return false;
}

socket_engine_base::socket_engine_base(cluster* creator) : owner(creator) {
#ifndef _WIN32
	set_signal_handler(SIGALRM);
	set_signal_handler(SIGXFSZ);
	set_signal_handler(SIGCHLD);
	signal(SIGHUP, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
#else
	// Set up winsock.
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		throw dpp::connection_exception(err_connect_failure, "WSAStartup failure");
	}
#endif
}

socket_engine_base::~socket_engine_base() {
#ifdef _WIN32
	WSACleanup();
#endif
}

time_t last_time = time(nullptr);

socket_events* socket_engine_base::get_fd(dpp::socket fd) {
	std::unique_lock lock(fds_mutex);
	auto iter = fds.find(fd);
	if (iter == fds.end()) {
		return nullptr;
	}
	return iter->second.get();
}

void socket_engine_base::inplace_modify_fd(dpp::socket fd, uint8_t extra_flags) {
	bool should_modify;
	socket_events s{};
	{
		std::lock_guard lk(fds_mutex);
		auto i = fds.find(fd);
		should_modify = i != fds.end() && (i->second->flags & extra_flags) != extra_flags;
		if (should_modify) {
			i->second->flags |= extra_flags;
			s = *(i->second);
		}
	}
	if (should_modify) {
		update_socket(s);
	}
}

void socket_engine_base::prune() {
	if (time(nullptr) != last_time) {
		try {
			owner->tick_timers();
		} catch (const std::exception& e) {
			owner->log(dpp::ll_error, "Uncaught exception in tick_timers: " + std::string(e.what()));
		}

		if ((time(nullptr) % 60) == 0) {
			/* Every minute, rehash all cache containers.
			 * We do this from the socket engine now, not from
			 * shard 0, so no need to run shards to have timers!
			 */
			dpp::garbage_collection();
		}

		last_time = time(nullptr);
	}
	stats.iterations++;
}

bool socket_engine_base::delete_socket(dpp::socket fd) {
	std::unique_lock lock(fds_mutex);
	auto iter = fds.find(fd);
	if (iter == fds.end() || ((iter->second->flags & WANT_DELETION) != 0L)) {
		return false;
	}
	iter->second->flags |= WANT_DELETION;
	stats.deletions++;
	stats.active_fds--;
	return true;
}

bool socket_engine_base::remove_socket(dpp::socket fd) {
	return true;
}

const socket_stats& socket_engine_base::get_stats() const {
	return stats;
}

}
