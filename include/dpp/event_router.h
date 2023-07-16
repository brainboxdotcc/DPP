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
#include <string>
#include <map>
#include <variant>
#include <dpp/snowflake.h>
#include <dpp/misc-enum.h>
#include <dpp/json_fwd.h>
#include <algorithm>
#include <mutex>
#include <shared_mutex>
#include <cstring>
#include <dpp/coro.h>

using  json = nlohmann::json;

namespace dpp {

/**
 * @brief A returned event handle for an event which was attached
 */
typedef size_t event_handle;

/**
 * @brief Handles routing of an event to multiple listeners.
 * 
 * Multiple listeners may attach to the event_router_t by means of operator(). Passing a
 * lambda into operator() attaches to the event.
 * 
 * Dispatchers of the event may call the event_router_t::call() method to cause all listeners
 * to receive the event.
 * 
 * The event_router_t::empty() method will return true if there are no listeners attached
 * to the event_router_t (this can be used to save time by not constructing objects that
 * nobody will ever see).
 * 
 * The event_router_t::detach() method removes an existing listener from the event,
 * using the event_handle ID returned by operator().
 * 
 * This class is used by the library to route all websocket events to listening code.
 * 
 * Example:
 * 
 * ```cpp
 * // Declare an event that takes log_t as its parameter
 * event_router_t<log_t> my_event;
 * 
 * // Attach a listener to the event
 * event_handle id = my_event([&](const log_t& cc) {
 *	 std::cout << cc.message << "\n";
 * });
 * 
 * // Construct a log_t and call the event (listeners will receive the log_t object)
 * log_t lt;
 * lt.message = "foo";
 * my_event.call(lt);
 * 
 * // Detach from an event using the handle returned by operator()
 * my_event.detach(id);
 * ```
 * 
 * @tparam T type of single parameter passed to event lambda derived from event_dispatch_t
 */
template<class T> class event_router_t {
private:
	friend class cluster;

	event_handle next_handle = 1;

	/**
	 * @brief Thread safety mutex
	 */
	mutable std::shared_mutex lock;
	/**
	 * @brief Container of event listeners keyed by handle,
	 * as handles are handed out sequentially they will always
	 * be called in they order they are bound to the event
	 * as std::map is an ordered container.
	 */
	std::map<event_handle, std::function<void(const T&)>> dispatch_container;


#ifdef DPP_CORO
	/**
	 * @brief Container for event listeners (coroutines only)
	 *
	 * Note: keep a listener's parameter as a value type, the event passed can die while a coroutine is suspended
	 */
	std::map<event_handle, std::function<dpp::task<void>(T)>> coroutine_container;
#else
#ifndef _DOXYGEN_
	/**
	 * @brief Dummy container to keep the struct size same
	 */
	std::map<event_handle, std::function<void(T)>> dummy_container;
#endif /* _DOXYGEN_ */
#endif /* DPP_CORO */


	/**
	 * @brief A function to be called whenever the method is called, to check
	 * some condition that is required for this event to trigger correctly.
	 */
	std::function<void(const T&)> warning;

	/**
	 * @brief Next handle to be given out by the event router
	 */

protected:

	/**
	 * @brief Set the warning callback object used to check that this
	 * event is capable of running properly
	 * 
	 * @param warning_function A checking function to call
	 */
	void set_warning_callback(std::function<void(const T&)> warning_function) {
		warning = warning_function;
	}

public:
	/**
	 * @brief Construct a new event_router_t object.
	 */
	event_router_t() = default;

	/**
	 * @brief Call all attached listeners.
	 * Listeners may cancel, by calling the event.cancel method.
	 * 
	 * @param event Class to pass as parameter to all listeners.
	 */
	void call(const T& event) const {
		if (warning) {
			warning(event);
		}
		std::shared_lock l(lock);
		std::for_each(dispatch_container.begin(), dispatch_container.end(), [&](auto &ev) {
			if (!event.is_cancelled()) {
				ev.second(event);
			}
		});
#ifdef DPP_CORO
		auto coro_exception_handler = [from = event.from](std::exception_ptr ptr) {
			try {
				std::rethrow_exception(ptr);
			}
			catch (const std::exception &exception) {
				if (from && from->creator)
					from->creator->log(dpp::loglevel::ll_error, std::string{"Uncaught exception in event coroutine: "} + exception.what());
			}
		};
		std::for_each(coroutine_container.begin(), coroutine_container.end(), [&](auto &ev) {
			if (!event.is_cancelled()) {
				dpp::task<void> task = ev.second(event);

				task.on_exception(coro_exception_handler);
			}
		});
#endif  /* DPP_CORO */
	};

	/**
	 * @brief Returns true if the container of listeners is empty,
	 * i.e. there is nothing listening for this event right now.
	 * 
	 * @return true if there are no listeners
	 * @return false if there are some listeners
	 */
	bool empty() const {
		std::shared_lock l(lock);
#ifdef DPP_CORO
		return dispatch_container.empty() && coroutine_container.empty();
#else
		return dispatch_container.empty();
#endif /* DPP_CORO */
	}

	/**
	 * @brief Returns true if any listeners are attached.
	 * 
	 * This is the boolean opposite of event_router_t::empty().
	 * @return true if listeners are attached
	 * @return false if no listeners are attached
	 */
	operator bool() const {
		return !empty();
	}

	/**
	 * @brief Attach a lambda to the event, adding a listener.
	 * The lambda should follow the signature specified when declaring
	 * the event object and should take exactly one parameter derived
	 * from event_dispatch_t.
	 * 
	 * @param func Function lambda to attach to event
	 * @return event_handle An event handle unique to this event, used to
	 * detach the listener from the event later if necessary.
	 */
	event_handle operator()(std::function<void(const T&)> func) {
		return this->attach(func);
	}

	/**
	 * @brief Attach a lambda to the event, adding a listener.
	 * The lambda should follow the signature specified when declaring
	 * the event object and should take exactly one parameter derived
	 * from event_dispatch_t.
	 *
	 * @param func Function lambda to attach to event
	 * @return event_handle An event handle unique to this event, used to
	 * detach the listener from the event later if necessary.
	 */
	event_handle attach(std::function<void(const T&)> func) {
		std::unique_lock l(lock);
		event_handle h = next_handle++;
		dispatch_container.emplace(h, func);
		return h;
	}

#ifdef DPP_CORO
	/**
	 * @brief Attach a coroutine task to the event, adding a listener.
	 * The coroutine should follow the signature specified when declaring
	 * the event object and should take exactly one parameter derived
	 * from event_dispatch_t.
	 *
	 * @param func Coroutine task to attack to the event
	 * @return event_handle An event handle unique to this event, used to
	 * detach the listener from the event later if necessary.
	 */
	event_handle co_attach(std::function<dpp::task<void>(T)> func) {
		std::unique_lock l(lock);
		event_handle h = next_handle++;
		coroutine_container.emplace(h, func);
		return h;
	}
#endif /* DPP_CORO */
	/**
	 * @brief Detach a listener from the event using a previously obtained ID.
	 * 
	 * @param handle An ID obtained from event_router_t::operator()
	 * @return true The event was successfully detached
	 * @return false The ID is invalid (possibly already detached, or does not exist)
	 */
	bool detach(const event_handle& handle) {
		std::unique_lock l(lock);
#ifdef DPP_CORO
		return this->dispatch_container.erase(handle) || this->coroutine_container.erase(handle);
#else
		return this->dispatch_container.erase(handle);
#endif /* DPP_CORO */
	}
};

};
