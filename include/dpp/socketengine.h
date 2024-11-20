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

/**
 * @brief Types of IO events a socket may subscribe to.
 */
enum socket_event_flags : uint8_t {
	/**
	 * @brief Socket wants to receive events when it can be read from.
	 * This is provided by the underlying implementation.
	 */
	WANT_READ = 1,
	/**
	 * @brief Socket wants to receive events when it can be written to.
	 * This is provided by the underlying implementation, and will be
	 * a one-off event. If you want to receive ongoing write events you
	 * must re-request this event type each time.
	 */
	WANT_WRITE = 2,
	/**
	 * @brief Socket wants to receive events that indicate an error condition.
	 * Note that EOF (graceful close) is not an error condition and is indicated
	 * by errno being 0 and ::read() returning 0.
	 */
	WANT_ERROR = 4,
	/**
	 * @brief Socket should be removed as soon as is safe to do so. Generally, this is
	 * after the current iteration through the active event list.
	 */
	WANT_DELETION = 8,
};

/**
 * @brief Read ready event
 */
using socket_read_event = std::function<void(dpp::socket fd, const struct socket_events&)>;

/**
 * @brief Write ready event
 */
using socket_write_event = std::function<void(dpp::socket fd, const struct socket_events&)>;

/**
 * @brief Error event
 */
using socket_error_event = std::function<void(dpp::socket fd, const struct socket_events&, int error_code)>;

/**
 * @brief Represents an active socket event set in the socket engine.
 *
 * An event set contains a file descriptor, a set of event handler callbacks, and
 * a set of bitmask flags which indicate which events it wants to receive.
 * It is possible to quickly toggle event types on or off, as it is not always necessary
 * or desired to receive all events all the time, in fact doing so can cause an event
 * storm which will consume 100% CPU (e.g. if you request to receive write events all
 * the time).
 */
struct DPP_EXPORT socket_events {
	/**
	 * @brief File descriptor
	 *
	 * This should be a valid file descriptor created via ::socket().
	 */
	dpp::socket fd{INVALID_SOCKET};

	/**
	 * @brief Flag bit mask of values from dpp::socket_event_flags
	 */
	uint8_t flags{0};

	/**
	 * @brief Read ready event
	 * @note This function will be called from a different thread to that
	 * which adds the event set to the socket engine.
	 */
	socket_read_event on_read{};

	/**
	 * @brief Write ready event
	 * @note This function will be called from a different thread to that
	 * which adds the event set to the socket engine.
	 */
	socket_write_event on_write{};

	/**
	 * @brief Error event
	 * @note This function will be called from a different thread to that
	 * which adds the event set to the socket engine.
	 */
	socket_error_event on_error{};

	/**
	 * @brief Construct a new socket_events
	 * @param socket_fd file descriptor
	 * @param _flags initial flags bitmask
	 * @param read_event read ready event
	 * @param write_event write ready event
	 * @param error_event error event
	 */
	socket_events(dpp::socket socket_fd, uint8_t _flags, const socket_read_event& read_event, const socket_write_event& write_event = {}, const socket_error_event& error_event = {})
		: fd(socket_fd), flags(_flags), on_read(read_event), on_write(write_event), on_error(error_event) { }

};

/**
 * @brief Container of event sets keyed by socket file descriptor
 */
using socket_container = std::unordered_map<dpp::socket, std::unique_ptr<socket_events>>;

/**
 * @brief This is the base class for socket engines.
 * The actual implementation is OS specific and the correct implementation is detected by
 * CMake. It is then compiled specifically into DPP so only one implementation can exist
 * in the implementation. All implementations should behave identically to the user, abstracting
 * out implementation-specific behaviours (e.g. difference between edge and level triggered
 * event mechanisms etc).
 */
struct DPP_EXPORT socket_engine_base {
	/**
	 * @brief File descriptors, and their states
	 */
	socket_container fds;

	/**
	 * @brief Thread pool.
	 * Event calls go into the thread pool and are called as
	 * and when threads in the pool are available.
	 */
	std::unique_ptr<thread_pool> pool;

	/**
	 * @brief Number of file descriptors we are waiting to delete
	 */
	size_t to_delete_count{0};

	/**
	 * @brief Owning cluster
	 */
	class cluster* owner{nullptr};

	/**
	 * @brief Default constructor
	 * @param creator Owning cluster
	 */
	socket_engine_base(class cluster* creator);

	/**
	 * @brief Non-copyable
	 */
	socket_engine_base(const socket_engine_base&) = delete;

	/**
	 * @brief Non-copyable
	 */
	socket_engine_base(socket_engine_base&&) = delete;

	/**
	 * @brief Non-movable
	 */
	socket_engine_base& operator=(const socket_engine_base&) = delete;

	/**
	 * @brief Non-movable
	 */
	socket_engine_base& operator=(socket_engine_base&&) = delete;

	/**
	 * @brief Default destructor
	 */
	virtual ~socket_engine_base() = default;

	/**
	 * @brief Should be called repeatedly in a loop.
	 * Will run for a maximum of 1 second.
	 */
	virtual void process_events() = 0;

	/**
	 * @brief Register a new socket with the socket engine
	 * @param e Socket events
	 * @return true if socket was added
	 */
	virtual bool register_socket(const socket_events& e);

	/**
	 * @brief Update an existing socket in the socket engine
	 * @param e Socket events
	 * @return true if socket was updated
	 */
	virtual bool update_socket(const socket_events& e);

	/**
	 * @brief Delete a socket from the socket engine
	 * @note This will not remove the socket immediately. It will set the
	 * WANT_DELETION flag causing it to be removed as soon as is safe to do so
	 * (once all events associated with it are completed).
	 * @param e File descriptor
	 * @return true if socket was queued for deletion
	 */
	bool delete_socket(dpp::socket fd);

	/**
	 * @brief Iterate through the list of sockets and remove any
	 * with WANT_DELETION set. This will also call implementation-specific
	 * remove_socket() on each entry to be removed.
	 */
	void prune();
protected:

	virtual bool remove_socket(dpp::socket fd);
};

/**
 * @brief This is implemented by whatever derived form socket_engine takes
 * @param creator Creating cluster
 */
DPP_EXPORT std::unique_ptr<socket_engine_base> create_socket_engine(class cluster *creator);

#ifndef _WIN32
	void set_signal_handler(int signal);
#endif

};
