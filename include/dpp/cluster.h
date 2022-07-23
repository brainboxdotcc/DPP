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
#include <dpp/dispatcher.h>
#include <dpp/misc-enum.h>
#include <dpp/timer.h>
#include <dpp/nlohmann/json_fwd.hpp>
#include <dpp/discordclient.h>
#include <dpp/voiceregion.h>
#include <dpp/dtemplate.h>
#include <dpp/prune.h>
#include <dpp/auditlog.h>
#include <dpp/queues.h>
#include <dpp/cache.h>
#include <dpp/intents.h>
#include <dpp/sync.h>
#include <algorithm>
#include <iostream>
#include <shared_mutex>
#include <cstring>

using  json = nlohmann::json;

namespace dpp {

#ifdef _WIN32
	#ifdef _DEBUG
		extern "C" DPP_EXPORT void you_are_using_a_debug_build_of_dpp_on_a_release_project();
	#else
		extern "C" DPP_EXPORT void you_are_using_a_release_build_of_dpp_on_a_debug_project();
	#endif
#endif

struct DPP_EXPORT version_checker {
	version_checker() {
		#ifdef _WIN32
			#ifdef _DEBUG
				you_are_using_a_debug_build_of_dpp_on_a_release_project();
			#else
				you_are_using_a_release_build_of_dpp_on_a_debug_project();
			#endif
		#endif
	}
};

static version_checker dpp_vc;


/**
 * @brief A list of shards
 */
typedef std::map<uint32_t, class discord_client*> shard_list;

/**
 * @brief Represents the various information from the 'get gateway bot' api call
 */
struct DPP_EXPORT gateway {
	/// Gateway websocket url
	std::string url;

	/// Number of suggested shards to start
	uint32_t shards;

	/// Total number of sessions that can be started
	uint32_t session_start_total;

	/// How many sessions are left
	uint32_t session_start_remaining;

	/// How many seconds until the session start quota resets
	uint32_t session_start_reset_after;

	/// How many sessions can be started at the same time
	uint32_t session_start_max_concurrency;

	/**
	 * @brief Construct a new gateway object
	 *
	 * @param j JSON data to construct from
	 */
	gateway(nlohmann::json* j);

	/**
	 * @brief Construct a new gateway object
	 */
	gateway();

	/**
	 * @brief Fill this object from json
	 * 
	 * @param j json to fill from
	 * @return gateway& reference to self
	 */
	gateway& fill_from_json(nlohmann::json* j);
};

/**
 * @brief Confirmation object represents any true or false simple REST request
 *
 */
struct DPP_EXPORT confirmation {
	bool success;
};

/**
 * @brief A container for types that can be returned for a REST API call
 *
 */
typedef std::variant<
		confirmation,
		message,
		message_map,
		user,
		user_identified,
		user_map,
		guild_member,
		guild_member_map,
		channel,
		channel_map,
		thread_member,
		thread_member_map,
		guild,
		guild_map,
		guild_command_permissions,
		guild_command_permissions_map,
		role,
		role_map,
		invite,
		invite_map,
		dtemplate,
		dtemplate_map,
		emoji,
		emoji_map,
		ban,
		ban_map,
		voiceregion,
		voiceregion_map,
		integration,
		integration_map,
		webhook,
		webhook_map,
		prune,
		guild_widget,
		gateway,
		interaction,
		interaction_response,
		auditlog,
		slashcommand,
		slashcommand_map,
		stage_instance,
		sticker,
		sticker_map,
		sticker_pack,
		sticker_pack_map,
		application,
		application_map,
		connection,
		connection_map,
		thread,
		thread_map,
		scheduled_event,
		scheduled_event_map,
		event_member,
		event_member_map,
		automod_rule,
		automod_rule_map
	> confirmable_t;

/**
 * @brief The details of a field in an error response
 */
struct DPP_EXPORT error_detail {
	/**
	 * @brief Object name which is in error
	 */
	std::string object;
	/**
	 * @brief Field name which is in error
	 */
	std::string field;
	/**
	 * @brief Error code
	 */
	std::string code;
	/**
	 * @brief Error reason (full message)
	 */
	std::string reason;
};

/**
 * @brief The full details of an error from a REST response
 */
struct DPP_EXPORT error_info {
	/**
	 * @brief Error code
	 */
	uint32_t code = 0;
	/**
	 * @brief Error message
	 *
	 */
	std::string message;
	/**
	 * @brief Field specific error descriptions
	 */
	std::vector<error_detail> errors;
};

/**
 * @brief The results of a REST call wrapped in a convenient struct
 */
struct DPP_EXPORT confirmation_callback_t {
	/** Information about the HTTP call used to make the request */
	http_request_completion_t http_info;

	/** Value returned, wrapped in variant */
	confirmable_t value;

	/** Owner/creator of the callback object */
	const class cluster* bot;

	/**
	 * @brief Construct a new confirmation callback t object
	 */
	confirmation_callback_t() = default;

	/**
	 * @brief Construct a new confirmation callback t object
	 * 
	 * @param creator owning cluster object
	 */
	confirmation_callback_t(cluster* creator);

	/**
	 * @brief Construct a new confirmation callback object
	 *
	 * @param creator owning cluster object
	 * @param _value The value to encapsulate in the confirmable_t
	 * @param _http The HTTP metadata from the REST call
	 */
	confirmation_callback_t(cluster* creator, const confirmable_t& _value, const http_request_completion_t& _http);

	/**
	 * @brief Returns true if the call resulted in an error rather than a legitimate value in the
	 * confirmation_callback_t::value member.
	 *
	 * @return true There was an error who's details can be obtained by get_error()
	 * @return false There was no error
	 */
	bool is_error() const;

	/**
	 * @brief Get the error_info object.
	 * The error_info object contains the details of any REST error, if there is an error
	 * (to find out if there is an error check confirmation_callback_t::is_error())
	 *
	 * @return error_info The details of the error message
	 */
	error_info get_error() const;
};

/**
 * @brief A callback upon command completion
 */
typedef std::function<void(const confirmation_callback_t&)> command_completion_event_t;

/**
 * @brief Automatically JSON encoded HTTP result
 */
typedef std::function<void(json&, const http_request_completion_t&)> json_encode_t;

extern DPP_EXPORT event_handle _next_handle;

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
	event_router_t() {
	}

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
		event_handle h = _next_handle++;
		dispatch_container.emplace(h, func);
		return h;		
	}

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

/** @brief The cluster class represents a group of shards and a command queue for sending and
 * receiving commands from discord via HTTP. You should usually instantiate a cluster object
 * at the very least to make use of the library.
 */
class DPP_EXPORT cluster {

	friend class discord_client;
	friend class discord_voice_client;

	/**
	 * @brief queue system for commands sent to Discord, and any replies
	 */
	request_queue* rest;

	/**
	 * @brief queue system for arbitrary HTTP requests sent by the user to sites other than Discord
	 */
	request_queue* raw_rest;

	/**
	 * @brief True if to use compression on shards
	 */
	bool compressed;

	/**
	 * @brief Lock to prevent concurrent access to dm_channels
	 */
	std::mutex dm_list_lock;

	/**
	 * @brief Start time of cluster
	 */
	time_t start_time;

	/**
	 * @brief Active DM channels for the bot
	 */
	std::unordered_map<snowflake, snowflake> dm_channels;

	/**
	 * @brief Active shards on this cluster. Shard IDs may have gaps between if there
	 * are multiple clusters.
	 */
	shard_list shards;

	/**
	 * @brief List of all active registered timers
	 */
	timer_reg_t timer_list;

	/**
	 * @brief List of timers by time
	 */
	timer_next_t next_timer;

	/**
	 * @brief Tick active timers
	 */
	void tick_timers();

	/**
	 * @brief Reschedule a timer for its next tick
	 * 
	 * @param t Timer to reschedule
	 */
	void timer_reschedule(timer_t* t);
public:
	/**
	 * @brief Current bot token for all shards on this cluster and all commands sent via HTTP
	 */
	std::string token;

	/**
	 * @brief Last time the bot sent an IDENTIFY
	 */
	time_t last_identify;

	/**
	 * @brief Current bitmask of gateway intents
	 */
	uint32_t intents;

	/**
	 * @brief Total number of shards across all clusters
	 */
	uint32_t numshards;

	/**
	 * @brief ID of this cluster, between 0 and MAXCLUSTERS-1 inclusive
	 */
	uint32_t cluster_id;

	/**
	 * @brief Total number of clusters that are active
	 */
	uint32_t maxclusters;

	/**
	 * @brief REST latency (HTTPS ping) in seconds
	 */
	double rest_ping;

	/**
	 * @brief The details of the bot user. This is assumed to be identical across all shards
	 * in the cluster. Each connecting shard updates this information.
	 */
	dpp::user me;

	/**
	 * @brief Current cache policy for the cluster
	 */
	cache_policy_t cache_policy;

	/**
	 * @brief Websocket mode for all shards in the cluster, either ws_json or ws_etf.
	 * Production bots should use ETF, while development bots should use JSON.
	 */
	websocket_protocol_t ws_mode;

	/**
	 * @brief Condition variable notified when the cluster is terminating.
	 */
	std::condition_variable terminating;

	/**
	 * @brief Constructor for creating a cluster. All but the token are optional.
	 * @param token The bot token to use for all HTTP commands and websocket connections
	 * @param intents A bitmask of dpd::intents values for all shards on this cluster. This is required to be sent for all bots with over 100 servers.
	 * @param shards The total number of shards on this bot. If there are multiple clusters, then (shards / clusters) actual shards will run on this cluster.
	 * If you omit this value, the library will attempt to query the Discord API for the correct number of shards to start.
	 * @param cluster_id The ID of this cluster, should be between 0 and MAXCLUSTERS-1
	 * @param maxclusters The total number of clusters that are active, which may be on separate processes or even separate machines.
	 * @param compressed Whether or not to use compression for shards on this cluster. Saves a ton of bandwidth at the cost of some CPU
	 * @param policy Set the user caching policy for the cluster, either lazy (only cache users/members when they message the bot) or aggressive (request whole member lists on seeing new guilds too)
	 * @param request_threads The number of threads to allocate for making HTTP requests to Discord. This defaults to 12. You can increase this at runtime via the object returned from get_rest().
	 * @param request_threads_raw The number of threads to allocate for making HTTP requests to sites outside of Discord. This defaults to 1. You can increase this at runtime via the object returned from get_raw_rest().
	 * @throw dpp::exception Thrown on windows, if WinSock fails to initialise, or on any other system if a dpp::request_queue fails to construct
	 */
	cluster(const std::string &token, uint32_t intents = i_default_intents, uint32_t shards = 0, uint32_t cluster_id = 0, uint32_t maxclusters = 1, bool compressed = true, cache_policy_t policy = {cp_aggressive, cp_aggressive, cp_aggressive}, uint32_t request_threads = 12, uint32_t request_threads_raw = 1);

	/**
	 * @brief dpp::cluster is non-copyable
	 */
	cluster(const cluster&) = delete;

	/**
	 * @brief dpp::cluster is non-moveable
	 */
	cluster(const cluster&&) = delete;

	/**
	 * @brief Destroy the cluster object
	 */
	virtual ~cluster();

	/**
	 * @brief Get the rest_queue object which handles HTTPS requests to Discord
	 * @return request_queue* pointer to request_queue object
	 */
	request_queue* get_rest();

	/**
	 * @brief Get the raw rest_queue object which handles all HTTP(S) requests that are not directed at Discord
	 * @return request_queue* pointer to request_queue object
	 */
	request_queue* get_raw_rest();

	/**
	 * @brief Set the websocket protocol for all shards on this cluster.
	 * You should call this method before cluster::start.
	 * Generally ws_etf is faster, but provides less facilities for debugging should something
	 * go wrong. It is recommended to use ETF in production and JSON in development.
	 * 
	 * @param mode websocket protocol to use, either ws_json or ws_etf.
	 * @return cluster& Reference to self for chaining.
	 */
	cluster& set_websocket_protocol(websocket_protocol_t mode);

	/**
	 * @brief Set the audit log reason for the next REST call to be made.
	 * This is set per-thread, so you must ensure that if you call this method, your request that
	 * is associated with the reason happens on the same thread where you set the reason.
	 * Once the next call is made, the audit log reason is cleared for this thread automatically.
	 * 
	 * Example:
	 * ```
	 * bot.set_audit_reason("Too much abusive content")
	 *	.channel_delete(my_channel_id);
	 * ```
	 * 
	 * @param reason The reason to set for the next REST call on this thread
	 * @return cluster& Reference to self for chaining.
	 */
	cluster& set_audit_reason(const std::string &reason);

	/**
	 * @brief Clear the audit log reason for the next REST call to be made.
	 * This is set per-thread, so you must ensure that if you call this method, your request that
	 * is associated with the reason happens on the same thread where you set the reason.
	 * Once the next call is made, the audit log reason is cleared for this thread automatically.
	 * 
	 * Example:
	 * ```
	 * bot.set_audit_reason("Won't be sent")
	 *	.clear_audit_reason()
	 *	.channel_delete(my_channel_id);
	 * ```
	 * 
	 * @return cluster& Reference to self for chaining.
	 */
	cluster& clear_audit_reason();

	/**
	 * @brief Get the audit reason set for the next REST call to be made on this thread.
	 * This is set per-thread, so you must ensure that if you call this method, your request that
	 * is associated with the reason happens on the same thread where you set the reason.
	 * Once the next call is made, the audit log reason is cleared for this thread automatically.
	 * 
	 * @note This method call clears the audit reason when it returns it.
	 * 
	 * @return std::string The audit reason to be used.
	 *
	 */
	std::string get_audit_reason();

	/**
	 * @brief Log a message to whatever log the user is using.
	 * The logged message is passed up the chain to the on_log event in user code which can then do whatever
	 * it wants to do with it.
	 * @param severity The log level from dpp::loglevel
	 * @param msg The log message to output
	 */
	void log(dpp::loglevel severity, const std::string &msg) const;

	/**
	 * @brief Start a timer. Every `frequency` seconds, the callback is called.
	 * 
	 * @param on_tick The callback lambda to call for this timer when ticked
	 * @param on_stop The callback lambda to call for this timer when it is stopped
	 * @param frequency How often to tick the timer in seconds
	 * @return timer A handle to the timer, used to remove that timer later
	 */
	timer start_timer(timer_callback_t on_tick, uint64_t frequency, timer_callback_t on_stop = {});

	/**
	 * @brief Stop a ticking timer
	 * 
	 * @param t Timer handle received from cluster::start_timer
	 * @return bool True if the timer was stopped, false if it did not exist
	 * @note If the timer has an on_stop lambda, the on_stop lambda will be called.
	 */
	bool stop_timer(timer t);

	/**
	 * @brief Get the dm channel for a user id
	 *
	 * @param user_id the user id to get the dm channel for
	 * @return Returns 0 on failure
	 */
	snowflake get_dm_channel(snowflake user_id);

	/**
	 * @brief Set the dm channel id for a user id
	 *
	 * @param user_id user id to set the dm channel for
	 * @param channel_id dm channel to set
	 */
	void set_dm_channel(snowflake user_id, snowflake channel_id);

	/**
	 * @brief Returns the uptime of the cluster
	 *
	 * @return dpp::utility::uptime The uptime of the cluster
	 */
	dpp::utility::uptime uptime();

	/**
	 * @brief Start the cluster, connecting all its shards.
	 * 
	 * Returns once all shards are connected if return_after is true,
	 * otherwise enters an infinite loop while the shards run.
	 *
	 * @param return_after If true the bot will return to your program after starting shards, if false this function will never return.
	 */
	void start(bool return_after = true);

	/**
	 * @brief Set the presence for all shards on the cluster
	 *
	 * @param p The presence to set. Only the online status and the first activity are sent.
	 */
	void set_presence(const class dpp::presence &p);

	/**
	 * @brief Get a shard by id, returning the discord_client
	 *
	 * @param id Shard ID
	 * @return discord_client* shard, or null
	 */
	discord_client* get_shard(uint32_t id);

	/**
	 * @brief Get the list of shards
	 *
	 * @return shard_list& Reference to map of shards for this cluster
	 */
	const shard_list& get_shards();

	/* Functions for attaching to event handlers */

	/**
	 * @brief on voice state update event
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type voice_state_update_t&, and returns void.
	 */
	event_router_t<voice_state_update_t> on_voice_state_update;

	
	/**
	 * @brief on voice client disconnect event
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type voice_client_disconnect_t&, and returns void.
	 */
	event_router_t<voice_client_disconnect_t> on_voice_client_disconnect;

	
	/**
	 * @brief on voice client speaking event
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type voice_client_speaking_t&, and returns void.
	 */
	event_router_t<voice_client_speaking_t> on_voice_client_speaking;

	
	/**
	 * @brief Called when a log message is to be written to the log.
	 * You can attach any logging system here you wish, e.g. spdlog, or even just a simple
	 * use of std::cout or printf. If nothing attaches this log event, then the
	 * library will be silent.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type log_t&, and returns void.
	 */
	event_router_t<log_t> on_log;

	/**
	 * @brief on guild join request delete.
	 * Triggered when a user declines the membership screening questionnaire for a guild.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_join_request_delete_t&, and returns void.
	 */
	event_router_t<guild_join_request_delete_t> on_guild_join_request_delete;

	
	/**
	 * @brief Called when a new interaction is created.
	 * Interactions are created by discord when commands you have registered are issued
	 * by a user. For an example of this in action please see \ref slashcommands
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type interaction_create_t&, and returns void.
	 *
	 * @note There are dedicated events to handle slashcommands (See dpp::cluster::on_slashcommand),
	 * user context menus (See dpp::cluster::on_user_context_menu) and message context menus (See dpp::cluster::on_message_context_menu)
	 */
	event_router_t<interaction_create_t> on_interaction_create;

	/**
	 * @brief Called when a slash command is issued.
	 * Only dpp::ctxm_chat_input types of interaction are routed to this event.
	 * For an example of this in action please see \ref slashcommands
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type slashcommand_t&, and returns void.
	 */
	event_router_t<slashcommand_t> on_slashcommand;
	
	/**
	 * @brief Called when a button is clicked attached to a message.
	 * Button clicks are triggered by discord when buttons are clicked which you have
	 * associated with a message using dpp::component.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type button_click_t&, and returns void.
	 */
	event_router_t<button_click_t> on_button_click;
	
	/**
	 * @brief Called when an auto completed field needs suggestions to present to the user
	 * This is triggered by discord when option choices have auto completion enabled which you have
	 * associated with a dpp::slashcommand.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type autocomplete_t&, and returns void.
	 */
	event_router_t<autocomplete_t> on_autocomplete;

	
	/**
	 * @brief Called when a select menu is clicked attached to a message.
	 * Select menu clicks are triggered by discord when select menus are clicked which you have
	 * associated with a message using dpp::component.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type select_click_t&, and returns void.
	 */
	event_router_t<select_click_t> on_select_click;

	/**
	 * @brief Called when a user right-clicks or long-presses on a message,
	 * where a slash command is bound to the dpp::ctxm_message command type.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type message_context_menu_t&, and returns void.
	 */
	event_router_t<message_context_menu_t> on_message_context_menu;

	/**
	 * @brief Called when a user right-clicks or long-presses on a user,
	 * where a slash command is bound to the dpp::ctxm_user command type.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type user_context_menu_t&, and returns void.
	 */
	event_router_t<user_context_menu_t> on_user_context_menu;

	/**
	 * @brief Called when a modal dialog is submitted.
	 * Form submits are triggered by discord when modal dialogs are submitted which you have
	 * associated with a slash command using dpp::interaction_modal_response.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type form_submit_t&, and returns void.
	 */
	event_router_t<form_submit_t> on_form_submit;

	
	/**
	 * @brief Called when a guild is deleted.
	 * A guild can be deleted via the bot being kicked, the bot leaving the guild
	 * explicitly with dpp::cluster::guild_delete, or via the guild being unavailable due to
	 * an outage.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_delete_t&, and returns void.
	 */
	event_router_t<guild_delete_t> on_guild_delete;

	
	/**
	 * @brief Called when a channel is deleted from a guild.
	 * The channel will still be temporarily available in the cache. Pointers to the
	 * channel should not be retained long-term as they will be deleted by the garbage
	 * collector.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type channel_delete_t&, and returns void.
	 */
	event_router_t<channel_delete_t> on_channel_delete;

	
	/**
	 * @brief Called when a channel is edited on a guild.
	 * The new channel details have already been applied to the guild when you
	 * receive this event.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type channel_update_t&, and returns void.
	 */
	event_router_t<channel_update_t> on_channel_update;

	
	/**
	 * @brief Called when a shard is connected and ready.
	 * A set of cluster::on_guild_create events will follow this event.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type ready_t&, and returns void.
	 */
	event_router_t<ready_t> on_ready;

	
	/**
	 * @brief Called when a message is deleted.
	 * The message has already been deleted from Discord when you
	 * receive this event.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type message_delete_t&, and returns void.
	 */
	event_router_t<message_delete_t> on_message_delete;

	
	/**
	 * @brief Called when a user leaves a guild (either through being kicked, or choosing to leave)
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_member_remove_t&, and returns void.
	 */
	event_router_t<guild_member_remove_t> on_guild_member_remove;

	
	/**
	 * @brief Called when a connection to a shard successfully resumes.
	 * A resumed session does not need to re-synchronise guilds, members, etc.
	 * This is generally non-fatal and informational only.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type resumed_t&, and returns void.
	 */
	event_router_t<resumed_t> on_resumed;

	
	/**
	 * @brief Called when a new role is created on a guild.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_role_create_t&, and returns void.
	 */
	event_router_t<guild_role_create_t> on_guild_role_create;

	
	/**
	 * @brief Called when a user is typing on a channel.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type typing_start_t&, and returns void.
	 */
	event_router_t<typing_start_t> on_typing_start;

	
	/**
	 * @brief Called when a new reaction is added to a message.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type message_reaction_add_t&, and returns void.
	 */
	event_router_t<message_reaction_add_t> on_message_reaction_add;

	
	/**
	 * @brief Called when a set of members is received for a guild.
	 * D++ will request these for all new guilds if needed, after the cluster::on_guild_create
	 * events.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_members_chunk_t&, and returns void.
	 */
	event_router_t<guild_members_chunk_t> on_guild_members_chunk;

	
	/**
	 * @brief Called when a single reaction is removed from a message.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type message_reaction_remove_t&, and returns void.
	 */
	event_router_t<message_reaction_remove_t> on_message_reaction_remove;

	
	/**
	 * @brief Called when a new guild is created.
	 * D++ will request members for the guild for its cache using guild_members_chunk.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_create_t&, and returns void.
	 */
	event_router_t<guild_create_t> on_guild_create;

	
	/**
	 * @brief Called when a new channel is created on a guild.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type channel_create_t&, and returns void.
	 */
	event_router_t<channel_create_t> on_channel_create;

	
	/**
	 * @brief Called when all reactions for a particular emoji are removed from a message.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type message_reaction_remove_emoji_t&, and returns void.
	 */
	event_router_t<message_reaction_remove_emoji_t> on_message_reaction_remove_emoji;

	
	/**
	 * @brief Called when multiple messages are deleted from a channel or DM.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type message_delete_bulk_t&, and returns void.
	 */
	event_router_t<message_delete_bulk_t> on_message_delete_bulk;

	
	/**
	 * @brief Called when an existing role is updated on a guild.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_role_update_t&, and returns void.
	 */
	event_router_t<guild_role_update_t> on_guild_role_update;

	
	/**
	 * @brief Called when a role is deleted in a guild.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_role_delete_t&, and returns void.
	 */
	event_router_t<guild_role_delete_t> on_guild_role_delete;

	
	/**
	 * @brief Called when a message is pinned.
	 * Note that the pinned message is not returned to this event, just the timestamp
	 * of the last pinned message.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type channel_pins_update_t&, and returns void.
	 */
	event_router_t<channel_pins_update_t> on_channel_pins_update;

	
	/**
	 * @brief Called when all reactions are removed from a message.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type message_reaction_remove_all_t&, and returns void.
	 */
	event_router_t<message_reaction_remove_all_t> on_message_reaction_remove_all;

	
	/**
	 * @brief Called when we are told which voice server we can use.
	 * This will be sent either when we establish a new voice channel connection,
	 * or as discord rearrange their infrastructure.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type voice_server_update_t&, and returns void.
	 */
	event_router_t<voice_server_update_t> on_voice_server_update;

	
	/**
	 * @brief Called when new emojis are added to a guild.
	 * The complete set of emojis is sent every time.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_emojis_update_t&, and returns void.
	 */
	event_router_t<guild_emojis_update_t> on_guild_emojis_update;

	
	/**
	 * @brief Called when new stickers are added to a guild.
	 * The complete set of stickers is sent every time.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_stickers_update_t&, and returns void.
	 */
	event_router_t<guild_stickers_update_t> on_guild_stickers_update;

	
	/**
	 * @brief Called when a user's presence is updated.
	 * To receive these you will need the GUILD_PRESENCES privileged intent.
	 * You will receive many of these, very often, and receiving them will significantly
	 * increase your bot's CPU usage. If you don't need them it is recommended to not ask
	 * for them.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type presence_update_t&, and returns void.
	 */
	event_router_t<presence_update_t> on_presence_update;

	
	/**
	 * @brief Called when the webhooks for a guild are updated.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type webhooks_update_t&, and returns void.
	 */
	event_router_t<webhooks_update_t> on_webhooks_update;

	/**
	 * @brief Called when a new automod rule is created.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type automod_rule_create_t&, and returns void.
	 */
	event_router_t<automod_rule_create_t> on_automod_rule_create;


	/**
	 * @brief Called when an automod rule is updated.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type automod_rule_update_t&, and returns void.
	 */
	event_router_t<automod_rule_update_t> on_automod_rule_update;
	
	/**
	 * @brief Called when an automod rule is deleted.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type automod_rule_delete_t&, and returns void.
	 */
	event_router_t<automod_rule_delete_t> on_automod_rule_delete;

	/**
	 * @brief Called when an automod rule is triggered/executed.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type automod_rule_execute_t&, and returns void.
	 */
	event_router_t<automod_rule_execute_t> on_automod_rule_execute;

	/**
	 * @brief Called when a new member joins a guild.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_member_add_t&, and returns void.
	 */
	event_router_t<guild_member_add_t> on_guild_member_add;

	
	/**
	 * @brief Called when an invite is deleted from a guild.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type invite_delete_t&, and returns void.
	 */
	event_router_t<invite_delete_t> on_invite_delete;

	
	/**
	 * @brief Called when details of a guild are updated.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_update_t&, and returns void.
	 */
	event_router_t<guild_update_t> on_guild_update;

	
	/**
	 * @brief Called when an integration is updated for a guild.
	 * This returns the complete list.
	 * An integration is a connection to a guild of a user's associated accounts,
	 * e.g. youtube or twitch, for automatic assignment of roles etc.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_integrations_update_t&, and returns void.
	 */
	event_router_t<guild_integrations_update_t> on_guild_integrations_update;

	
	/**
	 * @brief Called when details of a guild member (e.g. their roles or nickname) are updated.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_member_update_t&, and returns void.
	 */
	event_router_t<guild_member_update_t> on_guild_member_update;

	
	/**
	 * @brief Called when a new invite is created for a guild.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type invite_create_t&, and returns void.
	 */
	event_router_t<invite_create_t> on_invite_create;

	
	/**
	 * @brief Called when a message is updated (edited).
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type message_update_t&, and returns void.
	 */
	event_router_t<message_update_t> on_message_update;

	
	/**
	 * @brief Called when a user is updated.
	 * This is separate to cluster::on_guild_member_update and includes things such as an avatar change,
	 * username change, discriminator change or change in subscription status for nitro.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type user_update_t&, and returns void.
	 */
	event_router_t<user_update_t> on_user_update;

	
	/**
	 * @brief Called when a new message arrives from discord.
	 * Note that D++ does not cache messages. If you want to cache these objects you
	 * should create something yourself within your bot. Caching of messages is not on
	 * the roadmap to be supported as it consumes excessive amounts of RAM.
	 * For an example for caching of messages, please see \ref caching-messages
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type message_create_t&, and returns void.
	 */
	event_router_t<message_create_t> on_message_create;

	
	/**
	 * @brief Called when a ban is added to a guild.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_ban_add_t&, and returns void.
	 */
	event_router_t<guild_ban_add_t> on_guild_ban_add;

	
	/**
	 * @brief Called when a ban is removed from a guild.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_ban_remove_t&, and returns void.
	 */
	event_router_t<guild_ban_remove_t> on_guild_ban_remove;

	
	/**
	 * @brief Called when a new integration is attached to a guild by a user.
	 * An integration is a connection to a guild of a user's associated accounts,
	 * e.g. youtube or twitch, for automatic assignment of roles etc.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type integration_create_t&, and returns void.
	 */
	event_router_t<integration_create_t> on_integration_create;

	
	/**
	 * @brief Called when an integration is updated by a user.
	 * This returns details of just the single integration that has changed.
	 * An integration is a connection to a guild of a user's associated accounts,
	 * e.g. youtube or twitch, for automatic assignment of roles etc.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type integration_update_t&, and returns void.
	 */
	event_router_t<integration_update_t> on_integration_update;

	
	/**
	 * @brief Called when an integration is removed by a user.
	 * An integration is a connection to a guild of a user's associated accounts,
	 * e.g. youtube or twitch, for automatic assignment of roles etc.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type integration_delete_t&, and returns void.
	 */
	event_router_t<integration_delete_t> on_integration_delete;

	
	/**
	 * @brief Called when a thread is created.
	 * Note that threads are not cached by D++, but a list of thread IDs is accessible in a guild object
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type thread_create_t&, and returns void.
	 */
	event_router_t<thread_create_t> on_thread_create;

	
	/**
	 * @brief Called when a thread is updated
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type thread_update_t&, and returns void.
	 */
	event_router_t<thread_update_t> on_thread_update;

	
	/**
	 * @brief Called when a thread is deleted
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type thread_delete_t&, and returns void.
	 */
	event_router_t<thread_delete_t> on_thread_delete;

	
	/**
	 * @brief Called when thread list is synced (upon gaining access to a channel).
	 * Note that threads are not cached by D++, but a list of thread IDs is accessible in a guild object
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type thread_list_sync_t&, and returns void.
	 */
	event_router_t<thread_list_sync_t> on_thread_list_sync;

	
	/**
	 * @brief Called when current user's thread member object is updated
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type thread_member_update_t&, and returns void.
	 */
	event_router_t<thread_member_update_t> on_thread_member_update;

	
	/**
	 * @brief Called when a thread's member list is updated (without GUILD_MEMBERS intent, is only called for current user)
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type thread_members_update_t&, and returns void.
	 */
	event_router_t<thread_members_update_t> on_thread_members_update;

	
	/**
	 * @brief Called when a new scheduled event is created
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_scheduled_event_create_t&, and returns void.
	 */
	event_router_t<guild_scheduled_event_create_t> on_guild_scheduled_event_create;

	
	/**
	 * @brief Called when a new scheduled event is updated
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_scheduled_event_update_t&, and returns void.
	 */
	event_router_t<guild_scheduled_event_update_t> on_guild_scheduled_event_update;

	
	/**
	 * @brief Called when a new scheduled event is deleted
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_scheduled_event_delete_t&, and returns void.
	 */
	event_router_t<guild_scheduled_event_delete_t> on_guild_scheduled_event_delete;

	
	/**
	 * @brief Called when a user is added to a scheduled event
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_scheduled_event_user_add_t&, and returns void.
	 */
	event_router_t<guild_scheduled_event_user_add_t> on_guild_scheduled_event_user_add;

	
	/**
	 * @brief Called when a user is removed to a scheduled event
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type guild_scheduled_event_user_remove_t&, and returns void.
	 */
	event_router_t<guild_scheduled_event_user_remove_t> on_guild_scheduled_event_user_remove;

	
	/**
	 * @brief Called when packets are sent from the voice buffer.
	 * The voice buffer contains packets that are already encoded with Opus and encrypted
	 * with Sodium, and merged into packets by the repacketizer, which is done in the
	 * dpp::discord_voice_client::send_audio method. You should use the buffer size properties
	 * of dpp::voice_buffer_send_t to determine if you should fill the buffer with more
	 * content.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type voice_buffer_send_t&, and returns void.
	 */
	event_router_t<voice_buffer_send_t> on_voice_buffer_send;

	
	/**
	 * @brief Called when a user is talking on a voice channel.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type voice_user_talking_t&, and returns void.
	 */
	event_router_t<voice_user_talking_t> on_voice_user_talking;

	
	/**
	 * @brief Called when a voice channel is connected and ready to send audio.
	 * Note that this is not directly attached to the READY event of the websocket,
	 * as there is further connection that needs to be done before audio is ready to send.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type voice_ready_t&, and returns void.
	 */
	event_router_t<voice_ready_t> on_voice_ready;

	
	/**
	 * @brief Called when new audio data is received.
	 * Each separate user's audio from the voice channel will arrive tagged with
	 * their user id in the event, if a user can be attributed to the received audio.
	 * 
	 * @note Receiving audio for bots is not officially supported by discord.
	 * 
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type voice_receive_t&, and returns void.
	 */
	event_router_t<voice_receive_t> on_voice_receive;

	/**
	 * @brief Called when new audio data is received, combined and mixed for all speaking users.
	 * 
	 * @note Receiving audio for bots is not officially supported by discord.
	 * 
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type voice_receive_t&, and returns void.
	 */
	event_router_t<voice_receive_t> on_voice_receive_combined;
	
	/**
	 * @brief Called when sending of audio passes over a track marker.
	 * Track markers are arbitrarily placed "bookmarks" in the audio buffer, placed
	 * by the bot developer. Each track marker can have a string value associated with it
	 * which is specified in dpp::discord_voice_client::insert_marker and returned to this
	 * event.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type voice_track_marker_t&, and returns void.
	 */
	event_router_t<voice_track_marker_t> on_voice_track_marker;

	
	/**
	 * @brief Called when a new stage instance is created on a stage channel.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * 
	 */
	event_router_t<stage_instance_create_t> on_stage_instance_create;

	
	/**
	 * @brief Called when a stage instance is updated.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type stage_instance_update_t&, and returns void.
	 */
	event_router_t<stage_instance_update_t> on_stage_instance_update;

	
	/**
	 * @brief Called when an existing stage instance is deleted from a stage channel.
	 *
	 * @note Use operator() to attach a lambda to this event, and the detach method to detach the listener using the returned ID.
	 * The function signature for this event takes a single `const` reference of type stage_instance_delete_t&, and returns void.
	 */
	event_router_t<stage_instance_delete_t> on_stage_instance_delete;

	
	/**
	 * @brief Post a REST request. Where possible use a helper method instead like message_create
	 *
	 * @param endpoint Endpoint to post to, e.g. /api/guilds
	 * @param major_parameters Major parameters for the endpoint e.g. a guild id
	 * @param parameters Minor parameters for the API request
	 * @param method Method, e.g. GET, POST
	 * @param postdata Post data (usually JSON encoded)
	 * @param callback Function to call when the HTTP call completes. The callback parameter will contain amongst other things, the decoded json.
	 * @param filename Filename to post for POST requests (for uploading files)
	 * @param filecontent File content to post for POST requests (for uploading files)
	 */
	void post_rest(const std::string &endpoint, const std::string &major_parameters, const std::string &parameters, http_method method, const std::string &postdata, json_encode_t callback, const std::string &filename = "", const std::string &filecontent = "");

	/**
	 * @brief Post a multipart REST request. Where possible use a helper method instead like message_create
	 *
	 * @param endpoint Endpoint to post to, e.g. /api/guilds
	 * @param major_parameters Major parameters for the endpoint e.g. a guild id
	 * @param parameters Minor parameters for the API request
	 * @param method Method, e.g. GET, POST
	 * @param postdata Post data (usually JSON encoded)
	 * @param callback Function to call when the HTTP call completes. The callback parameter will contain amongst other things, the decoded json.
	 * @param filename List of filenames to post for POST requests (for uploading files)
	 * @param filecontent List of file content to post for POST requests (for uploading files)
	 */
	void post_rest_multipart(const std::string &endpoint, const std::string &major_parameters, const std::string &parameters, http_method method, const std::string &postdata, json_encode_t callback, const std::vector<std::string> &filename = {}, const std::vector<std::string> &filecontent = {});

	/**
	 * @brief Make a HTTP(S) request. For use when wanting asynchronous access to HTTP APIs outside of Discord.
	 *
	 * @param url Full URL to post to, e.g. https://api.somewhere.com/v1/foo/
	 * @param method Method, e.g. GET, POST
	 * @param callback Function to call when the HTTP call completes. No processing is done on the returned data.
	 * @param postdata POST data
	 * @param mimetype MIME type of POST data
	 * @param headers Headers to send with the request
	 */
	void request(const std::string &url, http_method method, http_completion_event callback, const std::string &postdata = "", const std::string &mimetype = "text/plain", const std::multimap<std::string, std::string> &headers = {});

	/**
	 * @brief Respond to a slash command
	 *
	 * @see https://discord.com/developers/docs/interactions/receiving-and-responding#create-interaction-response
	 * @param interaction_id Interaction id to respond to
	 * @param token Token for the interaction webhook
	 * @param r Response to send
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void interaction_response_create(snowflake interaction_id, const std::string &token, const interaction_response &r, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit response to a slash command
	 *
	 * @see https://discord.com/developers/docs/interactions/receiving-and-responding#edit-original-interaction-response
	 * @param token Token for the interaction webhook
	 * @param m Message to send
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void interaction_response_edit(const std::string &token, const message &m, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Create a followup message to a slash command
	 * 
	 * @param token Token for the interaction webhook
	 * @param m followup message to create
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void interaction_followup_create(const std::string &token, const message &m, command_completion_event_t callback);

	/**
	 * @brief Edit original followup message to a slash command
	 * This is an alias for cluster::interaction_response_edit
	 * @see cluster::interaction_response_edit
	 * 
	 * @param token Token for the interaction webhook
	 * @param m message to edit, the ID should be set
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void interaction_followup_edit_original(const std::string &token, const message &m, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief 
	 * 
	 * @param token Token for the interaction webhook
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void interaction_followup_delete(const std::string &token, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit followup message to a slash command
	 * The message ID in the message you pass should be correctly set to that of a followup message you previously sent
	 * @param token Token for the interaction webhook
	 * @param m message to edit, the ID should be set
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void interaction_followup_edit(const std::string &token, const message &m, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get the followup message to a slash command
	 * @param token Token for the interaction webhook
	 * @param message_id message to retrieve
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void interaction_followup_get(const std::string &token, snowflake message_id, command_completion_event_t callback);

	/**
	 * @brief Create a global slash command (a bot can have a maximum of 100 of these).
	 * 
	 * @see https://discord.com/developers/docs/interactions/application-commands#create-global-application-command
	 * @param s Slash command to create
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::slashcommand object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void global_command_create(const slashcommand &s, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get a global slash command
	 *
	 * @see https://discord.com/developers/docs/interactions/application-commands#get-global-application-command
	 * @param id The ID of the slash command
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::slashcommand object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void global_command_get(snowflake id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get the audit log for a guild
	 *
	 * @see https://discord.com/developers/docs/resources/audit-log#get-guild-audit-log
	 * @param guild_id Guild to get the audit log of
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::auditlog object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_auditlog_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Create a slash command local to a guild
	 *
	 * @see https://discord.com/developers/docs/interactions/application-commands#create-guild-application-command
	 * @note Creating a command with the same name as an existing command for your application will overwrite the old command.
	 * @param s Slash command to create
	 * @param guild_id Guild ID to create the slash command in
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::slashcommand object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_command_create(const slashcommand &s, snowflake guild_id, command_completion_event_t callback = utility::log_error());


	/**
	 * @brief Create/overwrite guild slash commands.
	 * Any existing guild slash commands on this guild will be deleted and replaced with these.
	 *
	 * @see https://discord.com/developers/docs/interactions/application-commands#bulk-overwrite-guild-application-commands
	 * @param commands Vector of slash commands to create/update.
	 * New guild commands will be available in the guild immediately. If the command did not already exist, it will count toward daily application command create limits.
	 * @param guild_id Guild ID to create/update the slash commands in
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a list of dpp::slashcommand object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_bulk_command_create(const std::vector<slashcommand> &commands, snowflake guild_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Create/overwrite global slash commands.
	 * Any existing global slash commands will be deleted and replaced with these.
	 *
	 * @see https://discord.com/developers/docs/interactions/application-commands#bulk-overwrite-global-application-commands
	 * @param commands Vector of slash commands to create/update.
	 * overwriting existing commands that are registered globally for this application. Updates will be available in all guilds after 1 hour.
	 * Commands that do not already exist will count toward daily application command create limits.
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a list of dpp::slashcommand object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void global_bulk_command_create(const std::vector<slashcommand> &commands, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit a global slash command (a bot can have a maximum of 100 of these)
	 *
	 * @see https://discord.com/developers/docs/interactions/application-commands#edit-global-application-command
	 * @param s Slash command to change
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void global_command_edit(const slashcommand &s, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get a slash command of a guild
	 *
	 * @see https://discord.com/developers/docs/interactions/application-commands#get-guild-application-command
	 * @note The returned slash commands will not have permissions set, you need to use a permissions getter e.g. dpp::guild_commands_get_permissions to get the guild command permissions
	 * @param id The ID of the slash command
	 * @param guild_id Guild ID to get the slash command from
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::slashcommand object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_command_get(snowflake id, snowflake guild_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit a slash command local to a guild
	 *
	 * @see https://discord.com/developers/docs/interactions/application-commands#edit-guild-application-command
	 * @param s Slash command to edit
	 * @param guild_id Guild ID to edit the slash command in
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_command_edit(const slashcommand &s, snowflake guild_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit slash command permissions of a guild
	 *
	 * @see https://discord.com/developers/docs/interactions/application-commands#edit-application-command-permissions
	 * @note You can only add up to 10 permission overwrites for a command
	 * @param s Slash command to edit the permissions for
	 * @param guild_id Guild ID to edit the slash command in
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_command_edit_permissions(const slashcommand &s, snowflake guild_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get the permissions for a slash command of a guild
	 *
	 * @see https://discord.com/developers/docs/interactions/application-commands#get-application-command-permissions
	 * @param id The ID of the slash command to get the permissions for
	 * @param guild_id Guild ID to get the permissions of
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_command_permissions object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_command_get_permissions(snowflake id, snowflake guild_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit/Overwrite the permissions of all existing slash commands in a guild
	 *
	 * @note You can only add up to 10 permission overwrites for a command
	 *
	 * @see https://discord.com/developers/docs/interactions/application-commands#batch-edit-application-command-permissions
	 * @warning The endpoint will overwrite all existing permissions for all commands of the application in a guild, including slash commands, user commands, and message commands. Meaning that if you forgot to pass a slash command, the permissions of it might be removed.
	 * @param commands A vector of slash commands to edit/overwrite the permissions for
	 * @param guild_id Guild ID to edit permissions of the slash commands in
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_command_permissions_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 * @deprecated This has been disabled with updates to Permissions v2. You can use guild_command_edit_permissions instead
	 */
	void guild_bulk_command_edit_permissions(const std::vector<slashcommand> &commands, snowflake guild_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete a global slash command (a bot can have a maximum of 100 of these)
	 *
	 * @see https://discord.com/developers/docs/interactions/application-commands#delete-global-application-command
	 * @param id Slash command to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void global_command_delete(snowflake id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete a slash command local to a guild
	 *
	 * @see https://discord.com/developers/docs/interactions/application-commands#delete-guild-application-command
	 * @param id Slash command to delete
	 * @param guild_id Guild ID to delete the slash command in
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_command_delete(snowflake id, snowflake guild_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get the application's slash commands for a guild
	 *
	 * @see https://discord.com/developers/docs/interactions/application-commands#get-guild-application-commands
	 * @note The returned slash commands will not have permissions set, you need to use a permissions getter e.g. dpp::guild_commands_get_permissions to get the guild command permissions
	 * @param guild_id Guild ID to get the slash commands for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::slashcommand_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_commands_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get all slash command permissions of a guild
	 *
	 * @see https://discord.com/developers/docs/interactions/application-commands#get-application-command-permissions
	 * @param guild_id Guild ID to get the slash commands permissions for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_command_permissions_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_commands_get_permissions(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get the application's global slash commands
	 *
	 * @see https://discord.com/developers/docs/interactions/application-commands#get-global-application-commands
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::slashcommand_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void global_commands_get(command_completion_event_t callback);

	/**
	 * @brief Create a direct message, also create the channel for the direct message if needed
	 *
	 * @see https://discord.com/developers/docs/resources/user#create-dm
	 * @see https://discord.com/developers/docs/resources/channel#create-message
	 * @param user_id User ID of user to send message to
	 * @param m Message object
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void direct_message_create(snowflake user_id, const message &m, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get a message
	 *
	 * @see https://discord.com/developers/docs/resources/channel#get-channel-message
	 * @param message_id Message ID
	 * @param channel_id Channel ID
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_get(snowflake message_id, snowflake channel_id, command_completion_event_t callback);

	/**
	 * @brief Get multiple messages.
	 * 
	 * This function will attempt to fetch as many messages as possible using multiple API calls if needed.
	 *
	 * @see https://discord.com/developers/docs/resources/channel#get-channel-messages
	 * @param channel_id Channel ID to retrieve messages for
	 * @param around Messages should be retrieved around this ID if this is set to non-zero
	 * @param before Messages before this ID should be retrieved if this is set to non-zero
	 * @param after Messages after this ID should be retrieved if this is set to non-zero
	 * @param limit This number of messages maximum should be returned, up to a maximum of 100.
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void messages_get(snowflake channel_id, snowflake around, snowflake before, snowflake after, uint64_t limit, command_completion_event_t callback);

	/**
	 * @brief Send a message to a channel. The callback function is called when the message has been sent
	 *
	 * @see https://discord.com/developers/docs/resources/channel#create-message
	 * @param m Message to send
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_create(const struct message &m, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Crosspost a message. The callback function is called when the message has been sent
	 *
	 * @see https://discord.com/developers/docs/resources/channel#crosspost-message
	 * @param message_id Message to crosspost
	 * @param channel_id Channel ID to crosspost from
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_crosspost(snowflake message_id, snowflake channel_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit a message on a channel. The callback function is called when the message has been edited
	 *
	 * @see https://discord.com/developers/docs/resources/channel#edit-message
	 * @param m Message to edit
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_edit(const struct message &m, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Add a reaction to a message. The reaction string must be either an `emojiname:id` or a unicode character.
	 *
	 * @see https://discord.com/developers/docs/resources/channel#create-reaction
	 * @param m Message to add a reaction to
	 * @param reaction Reaction to add. Emojis should be in the form emojiname:id
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_add_reaction(const struct message &m, const std::string &reaction, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete own reaction from a message. The reaction string must be either an `emojiname:id` or a unicode character.
	 *
	 * @see https://discord.com/developers/docs/resources/channel#delete-own-reaction
	 * @param m Message to delete own reaction from
	 * @param reaction Reaction to delete. The reaction should be in the form emojiname:id
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete_own_reaction(const struct message &m, const std::string &reaction, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete a user's reaction from a message. The reaction string must be either an `emojiname:id` or a unicode character
	 *
	 * @see https://discord.com/developers/docs/resources/channel#delete-user-reaction
	 * @param m Message to delete a user's reaction from
	 * @param user_id User ID who's reaction you want to remove
	 * @param reaction Reaction to remove. Reactions should be in the form emojiname:id
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete_reaction(const struct message &m, snowflake user_id, const std::string &reaction, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get reactions on a message for a particular emoji. The reaction string must be either an `emojiname:id` or a unicode character
	 *
	 * @see https://discord.com/developers/docs/resources/channel#get-reactions
	 * @param m Message to get reactions for
	 * @param reaction Reaction should be in the form emojiname:id or a unicode character
	 * @param before Reactions before this ID should be retrieved if this is set to non-zero
	 * @param after Reactions before this ID should be retrieved if this is set to non-zero
	 * @param limit This number of reactions maximum should be returned
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::user_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_get_reactions(const struct message &m, const std::string &reaction, snowflake before, snowflake after, snowflake limit, command_completion_event_t callback);

	/**
	 * @brief Delete all reactions on a message
	 *
	 * @see https://discord.com/developers/docs/resources/channel#delete-all-reactions
	 * @param m Message to delete reactions from
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete_all_reactions(const struct message &m, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete all reactions on a message using a particular emoji. The reaction string must be either an `emojiname:id` or a unicode character
	 *
	 * @see https://discord.com/developers/docs/resources/channel#delete-all-reactions-for-emoji
	 * @param m Message to delete reactions from
	 * @param reaction Reaction to delete, in the form emojiname:id or a unicode character
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete_reaction_emoji(const struct message &m, const std::string &reaction, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Add a reaction to a message by id. The reaction string must be either an `emojiname:id` or a unicode character.
	 *
	 * @see https://discord.com/developers/docs/topics/gateway#message-reaction-add
	 * @param message_id Message to add reactions to
	 * @param channel_id Channel to add reactions to
	 * @param reaction Reaction to add. Emojis should be in the form emojiname:id
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_add_reaction(snowflake message_id, snowflake channel_id, const std::string &reaction, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete own reaction from a message by id. The reaction string must be either an `emojiname:id` or a unicode character.
	 *
	 * @see https://discord.com/developers/docs/resources/channel#delete-own-reaction
	 * @param message_id Message to delete reactions from
	 * @param channel_id Channel to delete reactions from
	 * @param reaction Reaction to delete. The reaction should be in the form emojiname:id
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete_own_reaction(snowflake message_id, snowflake channel_id, const std::string &reaction, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete a user's reaction from a message by id. The reaction string must be either an `emojiname:id` or a unicode character
	 *
	 * @see https://discord.com/developers/docs/resources/channel#delete-user-reaction
	 * @param message_id Message to delete reactions from
	 * @param channel_id Channel to delete reactions from
	 * @param user_id User ID who's reaction you want to remove
	 * @param reaction Reaction to remove. Reactions should be in the form emojiname:id
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete_reaction(snowflake message_id, snowflake channel_id, snowflake user_id, const std::string &reaction, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get reactions on a message for a particular emoji by id. The reaction string must be either an `emojiname:id` or a unicode character
	 *
	 * @see https://discord.com/developers/docs/resources/channel#get-reactions
	 * @param message_id Message to get reactions for
	 * @param channel_id Channel to get reactions for
	 * @param reaction Reaction should be in the form emojiname:id or a unicode character
	 * @param before Reactions before this ID should be retrieved if this is set to non-zero
	 * @param after Reactions before this ID should be retrieved if this is set to non-zero
	 * @param limit This number of reactions maximum should be returned
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::user_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_get_reactions(snowflake message_id, snowflake channel_id, const std::string &reaction, snowflake before, snowflake after, snowflake limit, command_completion_event_t callback);

	/**
	 * @brief Delete all reactions on a message by id
	 *
	 * @see https://discord.com/developers/docs/resources/channel#delete-all-reactions
	 * @param message_id Message to delete reactions from
	 * @param channel_id Channel to delete reactions from
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete_all_reactions(snowflake message_id, snowflake channel_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete all reactions on a message using a particular emoji by id. The reaction string must be either an `emojiname:id` or a unicode character
	 *
	 * @see https://discord.com/developers/docs/resources/channel#delete-all-reactions-for-emoji
	 * @param message_id Message to delete reactions from
	 * @param channel_id Channel to delete reactions from
	 * @param reaction Reaction to delete, in the form emojiname:id or a unicode character
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete_reaction_emoji(snowflake message_id, snowflake channel_id, const std::string &reaction, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete a message from a channel. The callback function is called when the message has been edited
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @see https://discord.com/developers/docs/resources/channel#delete-message
	 * @param message_id Message ID to delete
	 * @param channel_id Channel to delete from
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete(snowflake message_id, snowflake channel_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Bulk delete messages from a channel. The callback function is called when the message has been edited
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @note If any message provided older than 2 weeks or any duplicate message ID, it will fail.
	 *
	 * @see https://discord.com/developers/docs/resources/channel#bulk-delete-messages
	 * @param message_ids List of message IDs to delete (at least 2 and at most 100 message IDs)
	 * @param channel_id Channel to delete from
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete_bulk(const std::vector<snowflake> &message_ids, snowflake channel_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get a channel
	 *
	 * @see https://discord.com/developers/docs/resources/channel#get-channel
	 * @param c Channel ID to retrieve
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::channel object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_get(snowflake c, command_completion_event_t callback);

	/**
	 * @brief Get all channels for a guild
	 *
	 * @see https://discord.com/developers/docs/resources/channel#get-channels
	 * @param guild_id Guild ID to retrieve channels for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::channel_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channels_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Create a channel
	 * 
	 * Create a new channel object for the guild. Requires the `MANAGE_CHANNELS` permission. If setting permission overwrites,
	 * only permissions your bot has in the guild can be allowed/denied. Setting `MANAGE_ROLES` permission in channels is only possible
	 * for guild administrators. Returns the new channel object on success. Fires a `Channel Create Gateway` event.
	 * 
	 * All parameters to this endpoint are optional excluding `name`
	 * 
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @see https://discord.com/developers/docs/resources/channel#create-channel
	 * @param c Channel to create
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::channel object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_create(const class channel &c, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit a channel
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @see https://discord.com/developers/docs/resources/channel#modify-channel
	 * @param c Channel to edit/update
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::channel object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_edit(const class channel &c, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit multiple channels positions
	 * 
	 * Modify the positions of a set of channel objects for the guild.
	 * Requires `MANAGE_CHANNELS` permission. Fires multiple `Channel Update Gateway` events.
	 * Only channels to be modified are required.
	 *
	 * @see https://discord.com/developers/docs/resources/guild#modify-guild-channel-positions
	 * @param c Channel to change the position for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_edit_positions(const std::vector<channel> &c, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit a channel's permissions
	 *
	 * @see https://discord.com/developers/docs/resources/channel#edit-channel-permissions
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param c Channel to set permissions for
	 * @param overwrite_id Overwrite to change (a user or role ID)
	 * @param allow allow permissions bitmask
	 * @param deny deny permissions bitmask
	 * @param member true if the overwrite_id is a user id, false if it is a channel id
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_edit_permissions(const class channel &c, const snowflake overwrite_id, const uint64_t allow, const uint64_t deny, const bool member, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit a channel's permissions
	 *
	 * @see https://discord.com/developers/docs/resources/channel#edit-channel-permissions
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param channel_id ID of the channel to set permissions for
	 * @param overwrite_id Overwrite to change (a user or role ID)
	 * @param allow allow permissions bitmask
	 * @param deny deny permissions bitmask
	 * @param member true if the overwrite_id is a user id, false if it is a channel id
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_edit_permissions(const snowflake channel_id, const snowflake overwrite_id, const uint64_t allow, const uint64_t deny, const bool member, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete a channel
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @see https://discord.com/developers/docs/resources/channel#deleteclose-channel
	 * @param channel_id Channel id to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_delete(snowflake channel_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get details about an invite
	 *
	 * @see https://discord.com/developers/docs/resources/invite#get-invite
	 * @param invite Invite code to get information on
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::invite object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void invite_get(const std::string &invite, command_completion_event_t callback);

	/**
	 * @brief Delete an invite
	 *
	 * @see https://discord.com/developers/docs/resources/invite#delete-invite
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param invite Invite code to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::invite object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void invite_delete(const std::string &invite, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get invites for a channel
	 *
	 * @see https://discord.com/developers/docs/resources/invite#get-invites
	 * @param c Channel to get invites for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::invite_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_invites_get(const class channel &c, command_completion_event_t callback);

	/**
	 * @brief Create invite for a channel
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @see https://discord.com/developers/docs/resources/channel#create-channel-invite
	 * @param c Channel to create an invite on
	 * @param i Invite to create
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::invite object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_invite_create(const class channel &c, const class invite &i, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get a channel's pins
	 * @see https://discord.com/developers/docs/resources/channel#get-pinned-messages
	 * @param channel_id Channel ID to get pins for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_pins_get(snowflake channel_id, command_completion_event_t callback);

	/**
	 * @brief Adds a recipient to a Group DM using their access token
	 * @see https://discord.com/developers/docs/resources/channel#group-dm-add-recipient
	 * @param channel_id Channel id to add group DM recipients to
	 * @param user_id User ID to add
	 * @param access_token Access token from OAuth2
	 * @param nick Nickname of user to apply to the chat
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void gdm_add(snowflake channel_id, snowflake user_id, const std::string &access_token, const std::string &nick, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Removes a recipient from a Group DM
	 * @see https://discord.com/developers/docs/resources/channel#group-dm-remove-recipient
	 * @param channel_id Channel ID of group DM
	 * @param user_id User ID to remove from group DM
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void gdm_remove(snowflake channel_id, snowflake user_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Remove a permission from a channel
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @see https://discord.com/developers/docs/resources/channel#delete-channel-permission
	 * @param c Channel to remove permission from
	 * @param overwrite_id Overwrite to remove, user or channel ID
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_delete_permission(const class channel &c, snowflake overwrite_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Follow a news channel
	 * @see https://discord.com/developers/docs/resources/channel#follow-news-channel
	 * @param c Channel id to follow
	 * @param target_channel_id Channel to subscribe the channel to
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_follow_news(const class channel &c, snowflake target_channel_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Trigger channel typing indicator
	 * @see https://discord.com/developers/docs/resources/channel#trigger-typing-indicator
	 * @param c Channel to set as typing on
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_typing(const class channel &c, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Trigger channel typing indicator
	 * @see https://discord.com/developers/docs/resources/channel#trigger-typing-indicator
	 * @param cid Channel ID to set as typing on
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_typing(snowflake cid, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Pin a message
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @see https://discord.com/developers/docs/resources/channel#pin-message
	 * @param channel_id Channel id to pin message on
	 * @param message_id Message id to pin message on
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_pin(snowflake channel_id, snowflake message_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Unpin a message
	 * @see https://discord.com/developers/docs/resources/channel#unpin-message
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param channel_id Channel id to unpin message on
	 * @param message_id Message id to unpin message on
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_unpin(snowflake channel_id, snowflake message_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get a guild
	 * 
	 * Returns the guild object for the given id. This endpoint will also return approximate_member_count and approximate_presence_count
	 * for the guild.
	 * @see https://discord.com/developers/docs/resources/guild#get-guild
	 * @param g Guild ID to retrieve
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get(snowflake g, command_completion_event_t callback);

	/**
	 * @brief Get a guild preview. Returns a guild object but only a subset of the fields will be populated.
	 * 
	 * Returns the guild preview object for the given id `g`. If the user is not in the guild, then the guild
	 * must be lurkable (it must be Discoverable or have a live public stage).
	 * @see https://discord.com/developers/docs/resources/guild#get-guild-preview
	 * @param g Guild ID to retrieve
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_preview(snowflake g, command_completion_event_t callback);

	/**
	 * @brief Get a guild member
	 * @see https://discord.com/developers/docs/resources/guild#get-guild-member
	 * @param guild_id Guild ID to get member for
	 * @param user_id User ID of member to get
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_member object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_member(snowflake guild_id, snowflake user_id, command_completion_event_t callback);

	/**
	 * @brief Search for guild members based on whether their username or nickname starts with the given string.
	 *
	 * @note This endpoint is restricted according to whether the `GUILD_MEMBERS` Privileged Intent is enabled for your application.
	 * @see https://discord.com/developers/docs/resources/guild#search-guild-members
	 * @param guild_id Guild ID to search in
	 * @param query Query string to match username(s) and nickname(s) against
	 * @param limit max number of members to return (1-1000)
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_member_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_search_members(snowflake guild_id, const std::string& query, uint16_t limit, command_completion_event_t callback);

	/**
	 * @brief Get all guild members
	 * 
	 * @note This endpoint is restricted according to whether the `GUILD_MEMBERS` Privileged Intent is enabled for your application.
	 * @see https://discord.com/developers/docs/resources/guild#get-guild-members
	 * @param guild_id Guild ID to get all members for
	 * @param limit max number of members to return (1-1000)
	 * @param after the highest user id in the previous page
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_member_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_members(snowflake guild_id, uint16_t limit, snowflake after, command_completion_event_t callback);

	/**
	 * @brief Add guild member. Needs a specific oauth2 scope, from which you get the access_token.
	 * 
	 * Adds a user to the guild, provided you have a valid oauth2 access token for the user with the guilds.join scope.
	 * Returns the guild_member, which is defaulted if the user is already a member of the guild. Fires a `Guild Member Add` Gateway event.
	 * 
	 * For guilds with Membership Screening enabled, this endpoint will default to adding new members as pending in the guild member object.
	 * Members that are pending will have to complete membership screening before they become full members that can talk.
	 * 
	 * @note All parameters to this endpoint except for access_token are optional.
	 * The bot must be a member of the guild with `CREATE_INSTANT_INVITE` permission.
	 * @see https://discord.com/developers/docs/resources/guild#add-guild-member
	 * @param gm Guild member to add
	 * @param access_token Access token from Oauth2 scope
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_member object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_add_member(const guild_member& gm, const std::string &access_token, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit the properties of an existing guild member
	 * 
	 * Modify attributes of a guild member. Returns the guild_member. Fires a `Guild Member Update` Gateway event.
	 * If the `channel_id` is set to 0, this will force the target user to be disconnected from voice.
	 * To remove a timeout, set the `communication_disabled_until` to a non-zero time in the past, e.g. 1.
	 * When moving members to channels, the API user must have permissions to both connect to the channel and have the `MOVE_MEMBERS` permission.
	 * @see https://discord.com/developers/docs/resources/guild#modify-guild-member
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param gm Guild member to edit
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_member object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_edit_member(const guild_member& gm, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Moves the guild member to a other voice channel, if member is connected to one
	 *
	 * Fires a `Guild Member Update` Gateway event.
	 * @note When moving members to channels, the API user __must__ have permissions to both connect to the channel and have the `MOVE_MEMBERS` permission.
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @see https://discord.com/developers/docs/resources/guild#modify-guild-member
	 * @param channel_id Id of the channel to which the user is used
	 * @param guild_id Guild id to which the user is connected
	 * @param user_id User id, who should be moved
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_member object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_member_move(const snowflake channel_id, const snowflake guild_id, const snowflake user_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Change current user nickname
	 * 
	 * Modifies the nickname of the current user in a guild.
	 * Fires a `Guild Member Update` Gateway event.
	 * 
	 * @deprecated Deprecated in favor of Modify Current Member. Will be replaced by dpp::cluster::guild_current_member_edit
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @see https://discord.com/developers/docs/resources/guild#modify-current-user-nick
	 * @param guild_id Guild ID to change nickname on
	 * @param nickname New nickname, or empty string to clear nickname
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_set_nickname(snowflake guild_id, const std::string &nickname, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Add role to guild member
	 * 
	 * Adds a role to a guild member. Requires the `MANAGE_ROLES` permission.
	 * Fires a `Guild Member Update` Gateway event.
	 * @see https://discord.com/developers/docs/resources/guild#add-guild-member-role
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param guild_id Guild ID to add a role to
	 * @param user_id User ID to add role to
	 * @param role_id Role ID to add to the user
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_member_add_role(snowflake guild_id, snowflake user_id, snowflake role_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Remove role from guild member
	 * 
	 * Removes a role from a guild member. Requires the `MANAGE_ROLES` permission.
	 * Fires a `Guild Member Update` Gateway event.
	 * @see https://discord.com/developers/docs/resources/guild#remove-guild-member-role
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param guild_id Guild ID to remove role from user on
	 * @param user_id User ID to remove role from
	 * @param role_id Role to remove
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 * @deprecated Use dpp::cluster::guild_member_remove_role instead
	 */
	void guild_member_delete_role(snowflake guild_id, snowflake user_id, snowflake role_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Remove role from guild member
	 *
	 * Removes a role from a guild member. Requires the `MANAGE_ROLES` permission.
	 * Fires a `Guild Member Update` Gateway event.
	 * @see https://discord.com/developers/docs/resources/guild#remove-guild-member-role
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param guild_id Guild ID to remove role from user on
	 * @param user_id User ID to remove role from
	 * @param role_id Role to remove
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_member_remove_role(snowflake guild_id, snowflake user_id, snowflake role_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Remove (kick) a guild member
	 * 
	 * Remove a member from a guild. Requires `KICK_MEMBERS` permission.
	 * Fires a `Guild Member Remove` Gateway event.
	 * @see https://discord.com/developers/docs/resources/guild#remove-guild-member
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @deprecated Replaced by dpp::cluster::guild_member_kick
	 * @param guild_id Guild ID to kick member from
	 * @param user_id User ID to kick
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_member_delete(snowflake guild_id, snowflake user_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Remove (kick) a guild member
	 *  
	 * Remove a member from a guild. Requires `KICK_MEMBERS` permission.
	 * Fires a `Guild Member Remove` Gateway event.
	 * @see https://discord.com/developers/docs/resources/guild#remove-guild-member
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param guild_id Guild ID to kick member from
	 * @param user_id User ID to kick
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_member_kick(snowflake guild_id, snowflake user_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Set the timeout of a guild member
	 *
	 * Fires a `Guild Member Update` Gateway event.
	 * @see https://discord.com/developers/docs/resources/guild#modify-guild-member
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param guild_id Guild ID to timeout the member in
	 * @param user_id User ID to set the timeout for
	 * @param communication_disabled_until The timestamp when the user's timeout will expire (up to 28 days in the future). Set to 0 to remove the timeout
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_member_timeout(snowflake guild_id, snowflake user_id, time_t communication_disabled_until, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Add guild ban
	 *
	 * Create a guild ban, and optionally delete previous messages sent by the banned user.
	 * Requires the `BAN_MEMBERS` permission. Fires a `Guild Ban Add` Gateway event.
	 * @see https://discord.com/developers/docs/resources/guild#create-guild-ban
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param guild_id Guild ID to add ban to
	 * @param user_id User ID to ban
	 * @param delete_message_days How many days of their user's messages to also delete (0-7). Defaults to 0
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_ban_add(snowflake guild_id, snowflake user_id, uint32_t delete_message_days = 0, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete guild ban
	 * 
	 * Remove the ban for a user. Requires the `BAN_MEMBERS` permissions.
	 * Fires a Guild Ban Remove Gateway event.
	 * @see https://discord.com/developers/docs/resources/guild#remove-guild-ban
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param guild_id Guild to delete ban from
	 * @param user_id User ID to delete ban for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_ban_delete(snowflake guild_id, snowflake user_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get guild ban list
	 * 
	 * Requires the `BAN_MEMBERS` permission.
	 * @see https://discord.com/developers/docs/resources/guild#get-guild-bans
	 * @note Provide a user ID to `before` and `after` for pagination. Users will always be returned in ascending order by the user ID. If both before and after are provided, only before is respected.
	 * @param guild_id Guild ID to get bans for
	 * @param before If non-zero, all bans for user ids before this user id will be returned up to the limit
	 * @param after if non-zero, all bans for user ids after this user id will be returned up to the limit
	 * @param limit the maximum number of bans to retrieve in this call up to a maximum of 1000
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::ban_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_bans(snowflake guild_id, snowflake before, snowflake after, snowflake limit, command_completion_event_t callback);

	/**
	 * @brief Get single guild ban
	 * 
	 * Requires the `BAN_MEMBERS` permission.
	 * @see https://discord.com/developers/docs/resources/guild#get-guild-ban
	 * @param guild_id Guild ID to get ban for
	 * @param user_id User ID of ban to retrieve
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::ban object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_ban(snowflake guild_id, snowflake user_id, command_completion_event_t callback);

	/**
	 * @brief Get a template
	 * @see https://discord.com/developers/docs/resources/guild-template#get-guild-template
	 * @param code Template code
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::dtemplate object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void template_get(const std::string &code, command_completion_event_t callback);

	/**
	 * @brief Create a new guild based on a template.
	 * @note This endpoint can be used only by bots in less than 10 guilds.
	 * @see https://discord.com/developers/docs/resources/guild-template#create-guild-from-guild-template
	 * @param code Template code to create guild from
	 * @param name Guild name to create
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_create_from_template(const std::string &code, const std::string &name, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get guild templates
	 *
	 * @see https://discord.com/developers/docs/resources/guild-template#get-guild-templates
	 * @param guild_id Guild ID to get templates for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::dtemplate_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_templates_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Creates a template for the guild
	 *
	 * @see https://discord.com/developers/docs/resources/guild-template#create-guild-template
	 * @param guild_id Guild to create template from
	 * @param name Template name to create
	 * @param description Description of template to create
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::dtemplate object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_template_create(snowflake guild_id, const std::string &name, const std::string &description, command_completion_event_t callback);

	/**
	 * @brief Syncs the template to the guild's current state.
	 *
	 * @see https://discord.com/developers/docs/resources/guild-template#sync-guild-template
	 * @param guild_id Guild to synchronise template for
	 * @param code Code of template to synchronise
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::dtemplate object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_template_sync(snowflake guild_id, const std::string &code, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Modifies the template's metadata.
	 *
	 * @see https://discord.com/developers/docs/resources/guild-template#modify-guild-template
	 * @param guild_id Guild ID of template to modify
	 * @param code Template code to modify
	 * @param name New name of template
	 * @param description New description of template
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::dtemplate object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_template_modify(snowflake guild_id, const std::string &code, const std::string &name, const std::string &description, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Deletes the template
	 *
	 * @see https://discord.com/developers/docs/resources/guild-template#delete-guild-template
	 * @param guild_id Guild ID of template to delete
	 * @param code Template code to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_template_delete(snowflake guild_id, const std::string &code, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Create a guild
	 * 
	 * Create a new guild. Returns a guild object on success. `Fires a Guild Create Gateway` event.
	 * 
	 * When using the roles parameter, the first member of the array is used to change properties of the guild's everyone role.
	 * If you are trying to bootstrap a guild with additional roles, keep this in mind. The required id field within each role object is an
	 * integer placeholder, and will be replaced by the API upon consumption. Its purpose is to allow you to overwrite a role's permissions
	 * in a channel when also passing in channels with the channels array.
	 * When using the channels parameter, the position field is ignored, and none of the default channels are created. The id field within
	 * each channel object may be set to an integer placeholder, and will be replaced by the API upon consumption. Its purpose is to
	 * allow you to create `GUILD_CATEGORY` channels by setting the `parent_id` field on any children to the category's id field.
	 * Category channels must be listed before any children.
	 *
	 * @see https://discord.com/developers/docs/resources/guild#create-guild
	 * @note The region field is deprecated and is replaced by channel.rtc_region. This endpoint can be used only by bots in less than 10 guilds.
	 * @param g Guild to create
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_create(const class guild &g, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit a guild
	 * 
	 * Modify a guild's settings. Requires the `MANAGE_GUILD` permission. Returns the updated guild object on success.
	 * Fires a `Guild Update Gateway` event.
	 * 
	 * @see https://discord.com/developers/docs/resources/guild#modify-guild
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param g Guild to edit
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_edit(const class guild &g, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete a guild
	 * 
	 * Delete a guild permanently. User must be owner. Fires a `Guild Delete Gateway` event.
	 *
	 * @see https://discord.com/developers/docs/resources/guild#delete-guild
	 * @param guild_id Guild ID to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_delete(snowflake guild_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get all emojis for a guild
	 *
	 * @see https://discord.com/developers/docs/resources/emoji#get-guild-emojis
	 * @param guild_id Guild ID to get emojis for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::emoji_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_emojis_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get a single emoji
	 *
	 * @see https://discord.com/developers/docs/resources/emoji#get-guild-emoji
	 * @param guild_id Guild ID to get emoji for
	 * @param emoji_id Emoji ID to get
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::emoji object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_emoji_get(snowflake guild_id, snowflake emoji_id, command_completion_event_t callback);

	/**
	 * @brief Create single emoji.
	 * You must ensure that the emoji passed contained image data using the emoji::load_image() method.
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @see https://discord.com/developers/docs/resources/emoji#create-guild-emoji
	 * @param guild_id Guild ID to create emoji om
	 * @param newemoji Emoji to create
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::emoji object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_emoji_create(snowflake guild_id, const class emoji& newemoji, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit a single emoji.
	 * 
	 * You must ensure that the emoji passed contained image data using the emoji::load_image() method.
	 * @see https://discord.com/developers/docs/resources/emoji#get-guild-emoji
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param guild_id Guild ID to edit emoji on
	 * @param newemoji Emoji to edit
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::emoji object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_emoji_edit(snowflake guild_id, const class emoji& newemoji, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete a guild emoji
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @see https://discord.com/developers/docs/resources/emoji#delete-guild-emoji
	 * @param guild_id Guild ID to delete emoji on
	 * @param emoji_id Emoji ID to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_emoji_delete(snowflake guild_id, snowflake emoji_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get prune counts
	 * 
	 * Returns a prune object indicating the number of members that would be removed in a prune operation. Requires the `KICK_MEMBERS`
	 * permission. By default, prune will not remove users with roles. You can optionally include specific roles in your prune by providing the
	 * include_roles parameter. Any inactive user that has a subset of the provided role(s) will be counted in the prune and users with additional
	 * roles will not.
	 *
	 * @see https://discord.com/developers/docs/resources/guild#get-guild-prune-count
	 * @param guild_id Guild ID to count for pruning
	 * @param pruneinfo Pruning info
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::prune object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_prune_counts(snowflake guild_id, const struct prune& pruneinfo, command_completion_event_t callback);

	/**
	 * @brief Begin guild prune
	 * 
	 * Begin a prune operation. Requires the `KICK_MEMBERS` permission. Returns a prune object indicating the number of members
	 * that were removed in the prune operation. For large guilds it's recommended to set the `compute_prune_count` option to false, forcing
	 * 'pruned' to 0. Fires multiple `Guild Member Remove` Gateway events.
	 * By default, prune will not remove users with roles. You can optionally include specific roles in your prune by providing the `include_roles`
	 * parameter. Any inactive user that has a subset of the provided role(s) will be included in the prune and users with additional roles will not.
	 * 
	 * @see https://discord.com/developers/docs/resources/guild#begin-guild-prune
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param guild_id Guild ID to prune
	 * @param pruneinfo Pruning info
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::prune object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_begin_prune(snowflake guild_id, const struct prune& pruneinfo, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get guild voice regions.
	 * 
	 * Voice regions per guild are somewhat deprecated in preference of per-channel voice regions.
	 * Returns a list of voice region objects for the guild. Unlike the similar /voice route, this returns VIP servers when
	 * the guild is VIP-enabled.
	 *
	 * @see https://discord.com/developers/docs/resources/guild#get-guild-voice-regions
	 * @param guild_id Guild ID to get voice regions for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::voiceregion_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_voice_regions(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get guild invites
	 * 
	 * Returns a list of invite objects (with invite metadata) for the guild. Requires the `MANAGE_GUILD` permission.
	 *
	 * @see https://discord.com/developers/docs/resources/guild#get-guild-invites
	 * @param guild_id Guild ID to get invites for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::invite_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_invites(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get guild integrations
	 * 
	 * Requires the `MANAGE_GUILD` permission.
	 *
	 * @see https://discord.com/developers/docs/resources/guild#get-guild-integrations
	 * @param guild_id Guild ID to get integrations for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::integration_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_integrations(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Modify guild integration
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @see https://discord.com/developers/docs/resources/guild#modify-guild-integration
	 * @param guild_id Guild ID to modify integration for
	 * @param i Integration to modify
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::integration object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_modify_integration(snowflake guild_id, const class integration &i, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete guild integration
	 * 
	 * Delete the attached integration object for the guild. Deletes any associated webhooks and kicks the associated bot if there is one.
	 * Requires the `MANAGE_GUILD` permission. Fires a Guild Integrations Update Gateway event.
	 * 
	 * @see https://discord.com/developers/docs/resources/guild#delete-guild-integration
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param guild_id Guild ID to delete integration for
	 * @param integration_id Integration ID to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_delete_integration(snowflake guild_id, snowflake integration_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Sync guild integration
	 *
	 * @see https://discord.com/developers/docs/resources/guild#sync-guild-integration
	 * @param guild_id Guild ID to sync integration on
	 * @param integration_id Integration ID to synchronise
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_sync_integration(snowflake guild_id, snowflake integration_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get guild widget
	 * 
	 * Requires the `MANAGE_GUILD` permission.
	 *
	 * @see https://discord.com/developers/docs/resources/guild#get-guild-widget
	 * @param guild_id Guild ID to get widget for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_widget object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_widget(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Edit guild widget
	 * 
	 * Requires the `MANAGE_GUILD` permission.
	 *
	 * @see https://discord.com/developers/docs/resources/guild#modify-guild-widget
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param guild_id Guild ID to edit widget for
	 * @param gw New guild widget information
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_widget object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_edit_widget(snowflake guild_id, const class guild_widget &gw, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get guild vanity url, if enabled
	 * 
	 * Returns a partial dpp::invite object for guilds with that feature enabled. Requires the `MANAGE_GUILD` permission. code will be null if a vanity url for the guild is not set.
	 * @see https://discord.com/developers/docs/resources/guild#get-guild-vanity-url
	 * @param guild_id Guild to get vanity URL for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::invite object in confirmation_callback_t::value filled to match the vanity url. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_vanity(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Create a webhook
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @see https://discord.com/developers/docs/resources/webhook#create-webhook
	 * @param w Webhook to create
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::webhook object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void create_webhook(const class webhook &w, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get guild webhooks
	 * @see https://discord.com/developers/docs/resources/webhook#get-guild-webhooks
	 * @param guild_id Guild ID to get webhooks for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::webhook_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void get_guild_webhooks(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get channel webhooks
	 * @see https://discord.com/developers/docs/resources/webhook#get-guild-webhooks
	 * @param channel_id Channel ID to get webhooks for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::webhook_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void get_channel_webhooks(snowflake channel_id, command_completion_event_t callback);

	/**
	 * @brief Get webhook
	 * @see https://discord.com/developers/docs/resources/webhook#get-webhook
	 * @param webhook_id Webhook ID to get
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::webhook object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void get_webhook(snowflake webhook_id, command_completion_event_t callback);

	/**
	 * @brief Get webhook using token
	 * @see https://discord.com/developers/docs/resources/webhook#get-webhook-with-token
	 * @param webhook_id Webhook ID to retrieve
	 * @param token Token of webhook
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::webhook object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void get_webhook_with_token(snowflake webhook_id, const std::string &token, command_completion_event_t callback);

	/**
	 * @brief Edit webhook
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @see https://discord.com/developers/docs/resources/webhook#modify-webhook
	 * @param wh Webhook to edit
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::webhook object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void edit_webhook(const class webhook& wh, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit webhook with token (token is encapsulated in the webhook object)
	 * @see https://discord.com/developers/docs/resources/webhook#modify-webhook-with-token
	 * @param wh Webhook to edit (should include token)
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::webhook object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void edit_webhook_with_token(const class webhook& wh, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete a webhook
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @see https://discord.com/developers/docs/resources/webhook#delete-webhook
	 * @param webhook_id Webhook ID to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void delete_webhook(snowflake webhook_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete webhook with token
	 * @see https://discord.com/developers/docs/resources/webhook#delete-webhook-with-token
	 * @param webhook_id Webhook ID to delete
	 * @param token Token of webhook to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void delete_webhook_with_token(snowflake webhook_id, const std::string &token, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Execute webhook
	 *
	 * @see https://discord.com/developers/docs/resources/webhook#execute-webhook
	 * @param wh Webhook to execute
	 * @param m Message to send
	 * @param wait waits for server confirmation of message send before response, and returns the created message body
	 * @param thread_id Send a message to the specified thread within a webhook's channel. The thread will automatically be unarchived
	 * @param thread_name Name of thread to create (requires the webhook channel to be a forum channel)
	 * @param callback Function to call when the API call completes.
	 * @note If the webhook channel is a forum channel, you must provide either `thread_id` or `thread_name`. If `thread_id` is provided, the message will send in that thread. If `thread_name` is provided, a thread with that name will be created in the forum channel.
	 * On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void execute_webhook(const class webhook &wh, const struct message &m, bool wait = false, snowflake thread_id = 0, const std::string& thread_name = "", command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get webhook message
	 *
	 * @see https://discord.com/developers/docs/resources/webhook#get-webhook-message
	 * @param wh Webhook to get the original message for
	 * @param message_id The message ID
	 * @param thread_id ID of the thread the message is in
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void get_webhook_message(const class webhook &wh, snowflake message_id, snowflake thread_id = 0, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit webhook message
	 *
	 * When the content field is edited, the mentions array in the message object will be reconstructed from scratch based on
	 * the new content. The allowed_mentions field of the edit request controls how this happens. If there is no explicit
	 * allowed_mentions in the edit request, the content will be parsed with default allowances, that is, without regard to
	 * whether or not an allowed_mentions was present in the request that originally created the message.
	 * 
	 * @see https://discord.com/developers/docs/resources/webhook#edit-webhook-message
	 * @note the attachments array must contain all attachments that should be present after edit, including retained and new attachments provided in the request body.
	 * @param wh Webhook to edit message for
	 * @param m New message
	 * @param thread_id ID of the thread the message is in
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void edit_webhook_message(const class webhook &wh, const struct message &m, snowflake thread_id = 0, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete webhook message
	 *
	 * @see https://discord.com/developers/docs/resources/webhook#delete-webhook-message
	 * @param wh Webhook to delete message for
	 * @param message_id Message ID to delete
	 * @param thread_id ID of the thread the message is in
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void delete_webhook_message(const class webhook &wh, snowflake message_id, snowflake thread_id = 0, command_completion_event_t callback = utility::log_error());


	/**
	 * @brief Get a role for a guild
	 *
	 * @see https://discord.com/developers/docs/resources/guild#get-guild-roles
	 * @param guild_id Guild ID to get role for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::role_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void roles_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Create a role on a guild
	 * 
	 * Create a new role for the guild. Requires the `MANAGE_ROLES` permission. Returns the new role object on success.
	 * Fires a `Guild Role Create` Gateway event.
	 * 
	 * @see https://discord.com/developers/docs/resources/guild#create-guild-role
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param r Role to create (guild ID is encapsulated in the role object)
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::role object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void role_create(const class role &r, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit a role on a guild
	 * 
	 * Requires the `MANAGE_ROLES` permission. Returns the updated role on success. Fires a `Guild Role Update` Gateway event.
	 * 
	 * @see https://discord.com/developers/docs/resources/guild#modify-guild-role
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param r Role to edit
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::role object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void role_edit(const class role &r, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit multiple role's position in a guild. Returns a list of all roles of the guild on success.
	 *
	 * Modify the positions of a set of role objects for the guild. Requires the `MANAGE_ROLES` permission.
	 * Fires multiple `Guild Role Update` Gateway events.
	 *
	 * @see https://discord.com/developers/docs/resources/guild#modify-guild-role-positions
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param guild_id Guild ID to change the roles position on
	 * @param roles Vector of roles to change the positions of
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::role_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void roles_edit_position(snowflake guild_id, const std::vector<role> &roles, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete a role
	 * 
	 * Requires the `MANAGE_ROLES` permission. Fires a `Guild Role Delete` Gateway event.
	 * 
	 * @see https://discord.com/developers/docs/resources/guild#delete-guild-role
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @param guild_id Guild ID to delete the role on
	 * @param role_id Role ID to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void role_delete(snowflake guild_id, snowflake role_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get a user by id
	 *
	 * @see https://discord.com/developers/docs/resources/user#get-user
	 * @param user_id User ID to retrieve
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::user_identified object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 * @note The user_identified object is a subclass of dpp::user which contains further details if you have the oauth2 identify or email scopes.
	 * If you do not have these scopes, these fields are empty. You can safely convert a user_identified to user with `dynamic_cast`.
	 * @note unless you want something special from `dpp::user_identified` or you've turned off caching, you have no need to call this.
	 * Call `dpp::find_user` instead that looks up the user in the cache rather than a REST call.
	 */
	void user_get(snowflake user_id, command_completion_event_t callback);

	/**
	 * @brief Get current (bot) user
	 *
	 * @see https://discord.com/developers/docs/resources/user#get-current-user
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::user_identified object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 * @note The user_identified object is a subclass of dpp::user which contains further details if you have the oauth2 identify or email scopes.
	 * If you do not have these scopes, these fields are empty. You can safely convert a user_identified to user with `dynamic_cast`.
	 */
	void current_user_get(command_completion_event_t callback);

	/**
	 * @brief Get current (bot) application
	 *
	 * @see https://discord.com/developers/docs/topics/oauth2#get-current-bot-application-information
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::application object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void current_application_get(command_completion_event_t callback);

	/**
	 * @brief Modify current member
	 *
	 * Modifies the current member in a guild.
	 * Fires a `Guild Member Update` Gateway event.
	 *
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @see https://discord.com/developers/docs/resources/guild#modify-current-member
	 * @param guild_id Guild ID to change on
	 * @param nickname New nickname, or empty string to clear nickname
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_current_member_edit(snowflake guild_id, const std::string &nickname, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get current user's connections (linked accounts, e.g. steam, xbox).
	 * This call requires the oauth2 `connections` scope and cannot be executed
	 * against a bot token.
	 * @see https://discord.com/developers/docs/resources/user#get-user-connections
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::connection_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void current_user_connections_get(command_completion_event_t callback);

	/**
	 * @brief Get current (bot) user guilds
	 * @see https://discord.com/developers/docs/resources/user#get-current-user-guilds
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void current_user_get_guilds(command_completion_event_t callback);

	/**
	 * @brief Edit current (bot) user
	 *
	 * Modifies the current member in a guild. Returns the updated guild_member object on success.
	 * Fires a `Guild Member Update` Gateway event.
	 * @see https://discord.com/developers/docs/resources/user#modify-current-user
	 * @param nickname Nickname to set
	 * @param image_blob Avatar data to upload (NOTE: Very heavily rate limited!)
	 * @param type Type of image for avatar
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::user object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
 	 * @throw dpp::exception Image data is larger than the maximum size of 256 kilobytes
	 */
	void current_user_edit(const std::string &nickname, const std::string& image_blob = "", const image_type type = i_png, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get current user DM channels
	 * 
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::channel_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void current_user_get_dms(command_completion_event_t callback);

	/**
	 * @brief Create a dm channel
	 * @see https://discord.com/developers/docs/resources/user#create-dm
	 * @param user_id User ID to create DM channel with
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::channel object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void create_dm_channel(snowflake user_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Leave a guild
	 * @see https://discord.com/developers/docs/resources/user#leave-guild
	 * @param guild_id Guild ID to leave
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void current_user_leave_guild(snowflake guild_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Create a thread
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @see https://discord.com/developers/docs/resources/guild#create-guild-channel
	 * @param thread_name Name of the thread
	 * @param channel_id Channel in which thread to create
	 * @param auto_archive_duration Duration after which thread auto-archives. Can be set to - 60, 1440 (for boosted guilds can also be: 4320, 10080)
	 * @param thread_type Type of thread - GUILD_PUBLIC_THREAD, GUILD_NEWS_THREAD, GUILD_PRIVATE_THREAD
	 * @param invitable whether non-moderators can add other non-moderators to a thread; only available when creating a private thread
	 * @param rate_limit_per_user amount of seconds a user has to wait before sending another message (0-21600); bots, as well as users with the permission manage_messages, manage_thread, or manage_channel, are unaffected
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::thread object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void thread_create(const std::string& thread_name, snowflake channel_id, uint16_t auto_archive_duration, channel_type thread_type, bool invitable, uint16_t rate_limit_per_user, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Create a thread with a message (Discord: ID of a thread is same as message ID)
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @see https://discord.com/developers/docs/topics/thread
	 * @param thread_name Name of the thread
	 * @param channel_id Channel in which thread to create
	 * @param message_id message to start thread with
	 * @param auto_archive_duration Duration after which thread auto-archives. Can be set to - 60, 1440 (for boosted guilds can also be: 4320, 10080)
	 * @param rate_limit_per_user amount of seconds a user has to wait before sending another message (0-21600); bots, as well as users with the permission manage_messages, manage_thread, or manage_channel, are unaffected
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::thread object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void thread_create_with_message(const std::string& thread_name, snowflake channel_id, snowflake message_id, uint16_t auto_archive_duration, uint16_t rate_limit_per_user, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Join a thread
	 * @see https://discord.com/developers/docs/topics/thread
	 * @param thread_id Thread ID to join
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void current_user_join_thread(snowflake thread_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Leave a thread
	 * @see https://discord.com/developers/docs/topics/thread
	 * @param thread_id Thread ID to leave
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void current_user_leave_thread(snowflake thread_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Add a member to a thread
	 * @see https://discord.com/developers/docs/topics/thread
	 * @param thread_id Thread ID to add to
	 * @param user_id Member ID to add
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void thread_member_add(snowflake thread_id, snowflake user_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Remove a member from a thread
	 * @see https://discord.com/developers/docs/topics/thread
	 * @param thread_id Thread ID to remove from
	 * @param user_id Member ID to remove
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void thread_member_remove(snowflake thread_id, snowflake user_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get a thread member
	 * @see https://discord.com/developers/docs/topics/thread
	 * @param thread_id Thread to get member for
	 * @param user_id ID of the user to get
	 * @param callback Function to call when the API call completes
	 * On success the callback will contain a dpp::thread_member object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void thread_member_get(const snowflake thread_id, const snowflake user_id, command_completion_event_t callback);

	/**
	 * @brief Get members of a thread
	 * @see https://discord.com/developers/docs/topics/thread
	 * @param thread_id Thread to get members for
	 * @param callback Function to call when the API call completes
	 * On success the callback will contain a dpp::thread_member_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void thread_members_get(snowflake thread_id, command_completion_event_t callback);

	/**
	 * @brief Get active threads in a channel (Sorted by ID in descending order)
	 * @see https://discord.com/developers/docs/topics/thread
	 * @param channel_id Channel to get active threads for
	 * @param callback Function to call when the API call completes
	 * On success the callback will contain a dpp::thread_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void threads_get_active(snowflake channel_id, command_completion_event_t callback);

	/**
	 * @brief Get public archived threads in a channel (Sorted by archive_timestamp in descending order)
	 * @see https://discord.com/developers/docs/topics/thread
	 * @param channel_id Channel to get public archived threads for
	 * @param before_timestamp Get threads before this timestamp
	 * @param limit Number of threads to get
	 * @param callback Function to call when the API call completes
	 * On success the callback will contain a dpp::thread_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void threads_get_public_archived(snowflake channel_id, time_t before_timestamp, uint16_t limit, command_completion_event_t callback);

	/**
	 * @brief Get private archived threads in a channel (Sorted by archive_timestamp in descending order)
	 * @see https://discord.com/developers/docs/topics/thread
	 * @param channel_id Channel to get public archived threads for
	 * @param before_timestamp Get threads before this timestamp
	 * @param limit Number of threads to get
	 * @param callback Function to call when the API call completes
	 * On success the callback will contain a dpp::thread_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void threads_get_private_archived(snowflake channel_id,  time_t before_timestamp, uint16_t limit, command_completion_event_t callback);

	/**
	 * @brief Get private archived threads in a channel which current user has joined (Sorted by ID in descending order)

	 * @see https://discord.com/developers/docs/topics/thread
	 * @param channel_id Channel to get public archived threads for
	 * @param before_id Get threads before this id
	 * @param limit Number of threads to get
	 * @param callback Function to call when the API call completes
	 * On success the callback will contain a dpp::thread_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void threads_get_joined_private_archived(snowflake channel_id, snowflake before_id, uint16_t limit, command_completion_event_t callback);

	/**
	 * @brief Create a sticker in a guild
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @see https://discord.com/developers/docs/resources/sticker#create-guild-sticker
	 * @param s Sticker to create. Must have its guild ID set.
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::sticker object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_sticker_create(sticker &s, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Modify a sticker in a guild
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @see https://discord.com/developers/docs/resources/sticker#modify-guild-sticker
	 * @param s Sticker to modify. Must have its guild ID and sticker ID set.
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::sticker object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_sticker_modify(sticker &s, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete a sticker from a guild
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 * @see https://discord.com/developers/docs/resources/sticker#delete-guild-sticker
	 * @param sticker_id sticker ID to delete
	 * @param guild_id guild ID to delete from
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::sticker object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_sticker_delete(snowflake sticker_id, snowflake guild_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get a nitro sticker
	 * @see https://discord.com/developers/docs/resources/sticker#get-sticker
	 * @param id Id of sticker to get.
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::sticker object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void nitro_sticker_get(snowflake id, command_completion_event_t callback);

	/**
	 * @brief Get a guild sticker
	 * @see https://discord.com/developers/docs/resources/sticker#get-guild-sticker
	 * @param id Id of sticker to get.
	 * @param guild_id Guild ID of the guild where the sticker is
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::sticker object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_sticker_get(snowflake id, snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get all guild stickers
	 * @see https://discord.com/developers/docs/resources/sticker#get-guild-stickers
	 * @param guild_id Guild ID of the guild where the sticker is
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::sticker_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_stickers_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get sticker packs
	 * @see https://discord.com/developers/docs/resources/sticker#list-nitro-sticker-packs
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::sticker_pack_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void sticker_packs_get(command_completion_event_t callback);

	/**
	 * @brief Create a stage instance on a stage channel.
	 * @see https://discord.com/developers/docs/resources/stage-instance#create-stage-instance
	 * @param instance Stage instance to create
	 * @param callback User function to execute when the api call completes
	 * On success the callback will contain a dpp::stage_instance object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 */
	void stage_instance_create(const stage_instance& instance, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get the stage instance associated with the channel id, if it exists.
	 * @see https://discord.com/developers/docs/resources/stage-instance#get-stage-instance
	 * @param channel_id ID of the associated channel
	 * @param callback User function to execute when the api call completes
	 * On success the callback will contain a dpp::stage_instance object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void stage_instance_get(const snowflake channel_id, command_completion_event_t callback);

	/**
	 * @brief Edit a stage instance.
	 * @see https://discord.com/developers/docs/resources/stage-instance#modify-stage-instance
	 * @param instance Stage instance to edit
	 * @param callback User function to execute when the api call completes
	 * On success the callback will contain a dpp::stage_instance object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 */
	void stage_instance_edit(const stage_instance& instance, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete a stage instance.
	 * @see https://discord.com/developers/docs/resources/stage-instance#delete-stage-instance
	 * @param channel_id ID of the associated channel
	 * @param callback User function to execute when the api call completes
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 */
	void stage_instance_delete(const snowflake channel_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get all voice regions
	 * @see https://discord.com/developers/docs/resources/voice#list-voice-regions
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::voiceregion_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void get_voice_regions(command_completion_event_t callback);

	/**
	 * @brief Get the gateway information for the bot using the token
	 * @see https://discord.com/developers/docs/topics/gateway#get-gateway-bot
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::gateway object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void get_gateway_bot(command_completion_event_t callback);

	/**
	 * @brief Get all scheduled events for a guild
	 * @see https://discord.com/developers/docs/resources/guild-scheduled-event#list-scheduled-events-for-guild
	 * @param guild_id Guild to get events for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::scheduled_event_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_events_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get users RSVP'd to an event
	 *
	 * @see https://discord.com/developers/docs/resources/guild-scheduled-event#get-guild-scheduled-event-users
	 * @param guild_id Guild to get user list for
	 * @param event_id Guild to get user list for
	 * @param limit Maximum number of results to return
	 * @param before Return user IDs that fall before this ID, if provided
	 * @param after Return user IDs that fall after this ID, if provided
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::event_member_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_event_users_get(snowflake guild_id, snowflake event_id, command_completion_event_t callback, uint8_t limit = 100, snowflake before = 0, snowflake after = 0);

	/**
	 * @brief Create a scheduled event on a guild
	 *
	 * @see https://discord.com/developers/docs/resources/guild-scheduled-event#create-guild-scheduled-event
	 * @param event Event to create (guild ID must be populated)
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::scheduled_event_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_event_create(const scheduled_event& event, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete a scheduled event from a guild
	 *
	 * @see https://discord.com/developers/docs/resources/guild-scheduled-event#delete-guild-scheduled-event
	 * @param event_id Event ID to delete
	 * @param guild_id Guild ID of event to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_event_delete(snowflake event_id, snowflake guild_id, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit/modify a scheduled event on a guild
	 *
	 * @see https://discord.com/developers/docs/resources/guild-scheduled-event#modify-guild-scheduled-event
	 * @param event Event to create (event ID and guild ID must be populated)
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::scheduled_event_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_event_edit(const scheduled_event& event, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get a scheduled event for a guild
	 *
	 * @see https://discord.com/developers/docs/resources/guild-scheduled-event#get-guild-scheduled-event
	 * @param guild_id Guild to get event for
	 * @param event_id Event ID to get
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::scheduled_event object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_event_get(snowflake guild_id, snowflake event_id, command_completion_event_t callback);

	/**
	 * @brief Set the bot's voice state on a stage channel
	 * 
	 * **Caveats**
	 * 
	 * There are currently several caveats for this endpoint:
	 * 
	 * - `channel_id` must currently point to a stage channel.
	 * - current user must already have joined `channel_id`.
	 * - You must have the `MUTE_MEMBERS` permission to unsuppress yourself. You can always suppress yourself.
	 * - You must have the `REQUEST_TO_SPEAK` permission to request to speak. You can always clear your own request to speak.
	 * - You are able to set `request_to_speak_timestamp` to any present or future time.
	 *
	 * @see https://discord.com/developers/docs/resources/guild#modify-current-user-voice-state 
	 * @param guild_id Guild to set voice state on
	 * @param channel_id Stage channel to set voice state on
	 * @param callback Function to call when the API call completes.
	 * @param suppress True if the user's audio should be suppressed, false if it should not
	 * @param request_to_speak_timestamp The time at which we requested to speak, or 0 to clear the request. The time set here must be the current time or in the future.
	 * On success the callback will contain a dpp::scheduled_event object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 * @throw std::logic_exception You attempted to set a request_to_speak_timestamp in the past which is not the value of 0.
	 */
	void current_user_set_voice_state(snowflake guild_id, snowflake channel_id, bool suppress = false, time_t request_to_speak_timestamp = 0, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Set a user's voice state on a stage channel
	 *
	 * **Caveats**
	 * 
	 * There are currently several caveats for this endpoint:
	 * 
	 * - `channel_id` must currently point to a stage channel.
	 * - User must already have joined `channel_id`.
	 * - You must have the `MUTE_MEMBERS` permission. (Since suppression is the only thing that is available currently)
	 * - When unsuppressed, non-bot users will have their `request_to_speak_timestamp` set to the current time. Bot users will not.
	 * - When suppressed, the user will have their `request_to_speak_timestamp` removed.
	 * 
	 * @see https://discord.com/developers/docs/resources/guild#modify-user-voice-state
	 * @param user_id The user to set the voice state of
	 * @param guild_id Guild to set voice state on
	 * @param channel_id Stage channel to set voice state on
	 * @param callback Function to call when the API call completes.
	 * @param suppress True if the user's audio should be suppressed, false if it should not
	 * On success the callback will contain a dpp::scheduled_event object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void user_set_voice_state(snowflake user_id, snowflake guild_id, snowflake channel_id, bool suppress = false, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Get all auto moderation rules for a guild
	 * 
	 * @param guild_id Guild id of the auto moderation rule
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::automod_rule_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void automod_rules_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get a single auto moderation rule
	 * 
	 * @param guild_id Guild id of the auto moderation rule
	 * @param rule_id  Rule id to retrieve
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::automod_rule object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void automod_rule_get(snowflake guild_id, snowflake rule_id, command_completion_event_t callback);

	/**
	 * @brief Create an auto moderation rule
	 * 
	 * @param guild_id Guild id of the auto moderation rule
	 * @param r Auto moderation rule to create
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::automod_rule object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void automod_rule_create(snowflake guild_id, const automod_rule& r, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Edit an auto moderation rule
	 * 
	 * @param guild_id Guild id of the auto moderation rule
	 * @param r Auto moderation rule to edit. The rule's id must be set.
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::automod_rule object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void automod_rule_edit(snowflake guild_id, const automod_rule& r, command_completion_event_t callback = utility::log_error());

	/**
	 * @brief Delete an auto moderation rule
	 * 
	 * @param guild_id Guild id of the auto moderation rule
	 * @param rule_id Auto moderation rule id to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void automod_rule_delete(snowflake guild_id, snowflake rule_id, command_completion_event_t callback = utility::log_error());

#include <dpp/cluster_sync_calls.h>

};

/**
 * @brief A timed_listener is a way to temporarily attach to an event for a specific timeframe, then detach when complete.
 * A lambda may also be optionally called when the timeout is reached. Destructing the timed_listener detaches any attached
 * event listeners, and cancels any created timers, but does not call any timeout lambda.
 * 
 * @tparam attached_event Event within cluster to attach to within the cluster::dispatch member (dpp::dispatcher object)
 * @tparam listening_function Definition of lambda function that matches up with the attached_event.
 */
template <typename attached_event, class listening_function> class timed_listener 
{
private:
	/// Owning cluster
	cluster* owner;

	/// Duration of listen
	time_t duration;

	/// Reference to attached event in cluster
	//event_router_t<thread_member_update_t> on_thread_member_update;
	attached_event& ev;

	/// Timer handle
	timer th;

	/// Event handle
	event_handle listener_handle;
	
public:
	/**
	 * @brief Construct a new timed listener object
	 * 
	 * @param cl Owning cluster
	 * @param _duration Duration of timed event in seconds
	 * @param event Event to hook, e.g. cluster.on_message_create
	 * @param on_end An optional void() lambda to trigger when the timed_listener times out.
	 * Calling the destructor before the timeout is reached does not call this lambda.
	 * @param listener Lambda to receive events. Type must match up properly with that passed into the 'event' parameter.
	 */
	timed_listener(cluster* cl, uint64_t _duration, attached_event& event, listening_function listener, timer_callback_t on_end = {})
	: owner(cl), duration(_duration), ev(event)
	{
		/* Attach event */
		listener_handle = ev(listener);
		/* Create timer */
		th = cl->start_timer([this](dpp::timer timer_handle) {
			/* Timer has finished, detach it from event.
			 * Only allowed to tick once.
			 */
			ev.detach(listener_handle);
			owner->stop_timer(th);
		}, duration, on_end);
	}

	/**
	 * @brief Destroy the timed listener object
	 */
	~timed_listener() {
		/* Stop timer and detach event, but do not call on_end */
		ev.detach(listener_handle);
		owner->stop_timer(th);
	}
};

/**
 * @brief Collects objects from events during a specified time period.
 * 
 * This template must be specialised. There are premade specialisations which you can use
 * such as dpp::reaction_collector and dpp::message_collector. For these specialised instances
 * all you need to do is derive a simple class from them which implements collector::completed().
 * 
 * A collector will run for the specified number of seconds, attaching itself to the
 * given event. During this time any events pass through the collector and collector::filter().
 * This function can return a pointer to an object to allow a copy of that object to be stored
 * to a vector, or it can return nullptr to do nothing with that object. For example a collector
 * attached to on_message_create would receive an event with the type message_create_t, and from
 * this may decide to extract the message_create_t::msg structure, returning a pointer to it, or
 * instead may choose to return a nullptr.
 * 
 * When either the predetermined timeout is reached, or the collector::cancel() method is called,
 * or the collector is destroyed, the collector::completed() method is called, which will be
 * passed a list of collected objects in the order they were collected.
 * 
 * @tparam T parameter type of the event this collector will monitor
 * @tparam C object type this collector will store
 */
template<class T, class C> class collector
{
protected:
	/// Owning cluster
	class cluster* owner;
private:
	/// Timed listener
	timed_listener<event_router_t<T>, std::function<void(const T&)>>* tl;
	/// stored list
	std::vector<C> stored;
	/// Trigger flag
	bool triggered;
public:
	/**
	 * @brief Construct a new collector object.
	 * 
	 * The timer for the collector begins immediately on construction of the object.
	 * 
	 * @param cl Pointer to cluster which manages this collector
	 * @param duration Duration in seconds to run the collector for
	 * @param event Event to attach to, e.g. cluster::on_message_create
	 */
	collector(class cluster* cl, uint64_t duration, event_router_t<T> & event) : owner(cl), triggered(false) {
		std::function<void(const T&)> f = [this](const T& event) {
			const C* v = filter(event);
			if (v) {
				stored.push_back(*v);
			}
		};
		tl = new dpp::timed_listener<event_router_t<T>, std::function<void(const T&)>>(cl, duration, event, f, [this](dpp::timer timer_handle) {
			if (!triggered) {
				triggered = true;
				completed(stored);
			}
		});
	}

	/**
	 * @brief You must implement this function to receive the completed list of
	 * captured objects.
	 * @param list The list of captured objects in captured order
	 */
	virtual void completed(const std::vector<C>& list) = 0;

	/**
	 * @brief Filter the list of elements.
	 * 
	 * Every time an event is fired on the collector, this method wil be called
	 * to determine if we should add an object to the list or not. This function
	 * can then process the `element` value, extract the parts which are to be
	 * saved to a list (e.g. a dpp::message out of a dpp::message_create_t) and
	 * return it as the return value. Returning a value of nullptr causes no
	 * object to be stored.
	 * 
	 * Here is an example of how to filter messages which have specific text in them.
	 * This should be used with the specialised type dpp::message_collector
	 * 
	 * ```cpp
	 * virtual const dpp::message* filter(const dpp::message_create_t& m) {
		 *	 if (m.msg.content.find("something i want") != std::string::npos) {
	 *		 return &m.msg;
	 *	 } else {
	 *		 return nullptr;
	 *	 }
	 * }
	 * ```
	 * 
	 * @param element The event data to filter
	 * @return const C* Returned object or nullptr
	 */
	virtual const C* filter(const T& element) = 0;

	/**
	 * @brief Immediately cancels the collector.
	 * 
	 * Use this if you have met the conditions for which you are collecting objects
	 * early, e.g. you were watching for a message containing 'yes' or 'no' and have
	 * received it before the time is up.
	 * 
	 * @note Causes calling of the completed() method if it has not yet been called.
	 */
	virtual void cancel() {
		delete tl;
		tl = nullptr;
	}

	/**
	 * @brief Destroy the collector object.
	 * @note Causes calling of the completed() method if it has not yet been called.
	 */
	virtual ~collector() {
		delete tl;
	}
};


};
