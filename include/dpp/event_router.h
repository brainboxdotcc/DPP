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
#include <dpp/coro/job.h>

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
		for (const auto& [_, listener] : dispatch_container) {
			if (!event.is_cancelled()) {
				listener(event);
			}
		};
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
		return dispatch_container.empty();
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

#ifdef _DOXYGEN_
	/**
	 * @brief Attach a callable to the event, adding a listener.
	 * The callable should either be of the form `void(const T &)` or
	 * `dpp::job(T)` (the latter requires DPP_CORO to be defined),
	 * where T is the event type for this event router.
	 *
	 * This has the exact same behavior as using attach.
	 *
	 * @see attach
	 * @param fun Callable to attach to event
	 * @return event_handle An event handle unique to this event, used to
	 * detach the listener from the event later if necessary.
	 */
	template <typename F>
	event_handle operator()(F&& fun);

	/**
	 * @brief Attach a callable to the event, adding a listener.
	 * The callable should either be of the form `void(const T &)` or
	 * `dpp::job(T)` (the latter requires DPP_CORO to be defined),
	 * where T is the event type for this event router.
	 *
	 * @param fun Callable to attach to event
	 * @return event_handle An event handle unique to this event, used to
	 * detach the listener from the event later if necessary.
	 */
	template <typename F>
	event_handle attach(F&& fun);
#else /* not _DOXYGEN_ */
#  ifdef DPP_CORO
	/**
	 * @brief Attach a callable to the event, adding a listener.
	 * The callable should either be of the form `void(const T &)` or
	 * `dpp::job(T)`, where T is the event type for this event router.
	 *
	 * @param fun Callable to attach to event
	 * @return event_handle An event handle unique to this event, used to
	 * detach the listener from the event later if necessary.
	 */
	template <typename F>
	requires (utility::callable_returns<F, dpp::job, const T&> || utility::callable_returns<F, void, const T&>)
	event_handle operator()(F&& fun) {
		return this->attach(std::forward<F>(fun));
	}

	/**
	 * @brief Attach a callable to the event, adding a listener.
	 * The callable should either be of the form `void(const T &)` or
	 * `dpp::job(T)`, where T is the event type for this event router.
	 *
	 * @param fun Callable to attach to event
	 * @return event_handle An event handle unique to this event, used to
	 * detach the listener from the event later if necessary.
	 */
	template <typename F>
	requires (utility::callable_returns<F, dpp::job, const T&> || utility::callable_returns<F, void, const T&>)
	event_handle attach(F&& fun) {
		std::unique_lock l(lock);
		event_handle h = next_handle++;
		dispatch_container.emplace(h, std::forward<F>(fun));
		return h;
	}
#  else
	/**
	 * @brief Attach a callable to the event, adding a listener.
	 * The callable should be of the form `void(const T &)`
	 * where T is the event type for this event router.
	 *
	 * @param fun Callable to attach to event
	 * @return event_handle An event handle unique to this event, used to
	 * detach the listener from the event later if necessary.
	 */
	template <typename F>
	std::enable_if_t<utility::callable_returns_v<F, void, const T&>, event_handle> operator()(F&& fun) {
		return this->attach(std::forward<F>(fun));
	}

	/**
	 * @brief Attach a callable to the event, adding a listener.
	 * The callable should be of the form `void(const T &)`
	 * where T is the event type for this event router.
	 *
	 * @param fun Callable to attach to event
	 * @return event_handle An event handle unique to this event, used to
	 * detach the listener from the event later if necessary.
	 */
	template <typename F>
	std::enable_if_t<utility::callable_returns_v<F, void, const T&>, event_handle> attach(F&& fun) {
		std::unique_lock l(lock);
		event_handle h = next_handle++;
		dispatch_container.emplace(h, std::forward<F>(fun));
		return h;
	}
#  endif /* DPP_CORO */
#endif /* _DOXYGEN_ */
	/**
	 * @brief Detach a listener from the event using a previously obtained ID.
	 *
	 * @param handle An ID obtained from event_router_t::operator()
	 * @return true The event was successfully detached
	 * @return false The ID is invalid (possibly already detached, or does not exist)
	 */
	bool detach(const event_handle& handle) {
		std::unique_lock l(lock);
		return this->dispatch_container.erase(handle);
	}
};

} // namespace dpp
