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
#include <dpp/discord.h>
#include <dpp/dispatcher.h>
#include <dpp/timer.h>
#include <dpp/json_fwd.hpp>
#include <dpp/discordclient.h>
#include <dpp/queues.h>
#include <algorithm>
#include <iostream>

using  json = nlohmann::json;

namespace dpp {

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
		event_member_map
	> confirmable_t;

/**
 * @brief Detatch a listener from an event container
 * 
 * @tparam T event container type
 * @param container container to detach from
 * @param ptr handle to listener to remove
 * @return bool True if successfully removed listener
 */
template<typename T> bool detach(T container, const event_handle ptr) {
	auto i = container.find(ptr);
	if (i != container.end()) {
		container.erase(i);
		return true;
	}
	return false;
}

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
	/** Returned data type in confirmable_t, used to double check to avoid an exception if you wish */
	std::string type;
	/** Information about the HTTP call used to make the request */
	http_request_completion_t http_info;
	/** Value returned, wrapped in variant */
	confirmable_t value;

	/**
	 * @brief Construct a new confirmation callback t object
	 */
	confirmation_callback_t() = default;

	/**
	 * @brief Construct a new confirmation callback object
	 *
	 * @param _type The type of callback that is encapsulated in the confirmable_t
	 * @param _value The value to encapsulate in the confirmable_t
	 * @param _http The HTTP metadata from the REST call
	 */
	confirmation_callback_t(const std::string &_type, const confirmable_t& _value, const http_request_completion_t& _http);

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

/** @brief The cluster class represents a group of shards and a command queue for sending and
 * receiving commands from discord via HTTP. You should usually instantiate a cluster object
 * at the very least to make use of the library.
 */
class DPP_EXPORT cluster {

	friend class discord_client;

	/** queue system for commands sent to Discord, and any replies */
	request_queue* rest;

	/** queue system for arbitrary HTTP requests sent by the user to sites other than Discord */
	request_queue* raw_rest;

	/** True if to use compression on shards */
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
	 * @brief Next event handle to be handed out.
	 * Always incremental from 1, unique during execution.
	 */
	event_handle next_eh;

	/**
	 * @brief Accepts result from /gateway/bot REST API call and populates numshards with it
	 *
	 * @param shardinfo Received HTTP data from API call
	 * @throw dpp::exception Thrown if REST request to obtain shard count fails
	 */
	void auto_shard(const confirmation_callback_t &shardinfo);

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
	/** Current bot token for all shards on this cluster and all commands sent via HTTP */
	std::string token;

	/* Last time the bot sent an IDENTIFY */
	time_t last_identify;

	/** Current bitmask of gateway intents */
	uint32_t intents;

	/** Total number of shards across all clusters */
	uint32_t numshards;

	/** ID of this cluster, between 0 and MAXCLUSTERS-1 inclusive */
	uint32_t cluster_id;

	/** Total number of clusters that are active */
	uint32_t maxclusters;

	/** REST latency (HTTPS ping) */
	double rest_ping;

	/** Routes events from Discord back to user program code via std::functions */
	dpp::dispatcher dispatch;

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
	 * @brief Constructor for creating a cluster. All but the token are optional.
	 * @param token The bot token to use for all HTTP commands and websocket connections
	 * @param intents A bitmask of dpd::intents values for all shards on this cluster. This is required to be sent for all bots with over 100 servers.
	 * @param shards The total number of shards on this bot. If there are multiple clusters, then (shards / clusters) actual shards will run on this cluster.
	 * If you omit this value, the library will attempt to query the Discord API for the correct number of shards to start.
	 * @param cluster_id The ID of this cluster, should be between 0 and MAXCLUSTERS-1
	 * @param maxclusters The total number of clusters that are active, which may be on separate processes or even separate machines.
	 * @param compressed Whether or not to use compression for shards on this cluster. Saves a ton of bandwidth at the cost of some CPU
	 * @param policy Set the user caching policy for the cluster, either lazy (only cache users/members when they message the bot) or aggressive (request whole member lists on seeing new guilds too)
	 * @throw dpp::exception Thrown on windows, if WinSock fails to initialise, or on any other system if a dpp::request_queue fails to construct
	 */
	cluster(const std::string &token, uint32_t intents = i_default_intents, uint32_t shards = 0, uint32_t cluster_id = 0, uint32_t maxclusters = 1, bool compressed = true, cache_policy_t policy = {cp_aggressive, cp_aggressive, cp_aggressive});

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
	 * @brief Get the next handle ID to be used
	 * 
	 * @return event_handle next ID to use
	 */
	event_handle get_next_handle();

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
	 *    .channel_delete(my_channel_id);
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
	 *    .clear_audit_reason()
	 *    .channel_delete(my_channel_id);
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
	 * @param frequency How often to tick the timer
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
	 * Returns once all shards are connected.
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
	 * @param _voice_state_update User function to attach to event
	 * Event is called with the parameter type `const` dpp::voice_state_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_voice_state_update (std::function<void(const voice_state_update_t& _event)> _voice_state_update);
	/**
	 * @brief Detach listener from on_voice_state_update event
	 * 
	 * @param _voice_state_update Handle to remove from event, previously returned by dpp::cluster::on_voice_state_update()
	 * @return true on successful detach of listener
	 */
	bool detach_voice_state_update(const event_handle _voice_state_update);

	/**
	 * @brief on voice client disconnect event
	 *
	 * @param _voice_client_disconnect User function to attach to event
	 * Event is called with the parameter type `const` dpp::voice_client_disconnect_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_voice_client_disconnect (std::function<void(const voice_client_disconnect_t& _event)> _voice_client_disconnect);
	/**
	 * @brief Detach listener from on_voice_client_disconnect event
	 * 
	 * @param _voice_client_disconnect Handle to remove from event, previously returned by dpp::cluster::on_voice_client_disconnect()
	 * @return true on successful detach of listener
	 */
	bool detach_voice_client_disconnect(const event_handle _voice_client_disconnect);

	/**
	 * @brief on voice client speaking event
	 *
	 * @param _voice_client_speaking User function to attach to event
	 * Event is called with the parameter type `const` dpp::voice_client_speaking_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_voice_client_speaking (std::function<void(const voice_client_speaking_t& _event)> _voice_client_speaking);
	/**
	 * @brief Detach listener from on_voice_client_speaking event
	 * 
	 * @param _voice_client_speaking Handle to remove from event, previously returned by dpp::cluster::on_voice_client_speaking()
	 * @return true on successful detach of listener
	 */
	bool detach_voice_client_speaking(const event_handle _voice_client_speaking);

	/**
	 * @brief Called when a log message is to be written to the log.
	 * You can attach any logging system here you wish, e.g. spdlog, or even just a simple
	 * use of std::cout or printf. If nothing attaches this log event, then the
	 * library will be silent.
	 *
	 * @param _log  User function to attach to event
	 * Event is called with the parameter type `const` dpp::log_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_log (std::function<void(const log_t& _event)> _log);
	/**
	 * @brief Detach listener from on_log event
	 * 
	 * @param _log Handle to remove from event, previously returned by dpp::cluster::on_log()
	 * @return true on successful detach of listener
	 */
	bool detach_log(const event_handle _log);

	/**
	 * @brief on guild join request delete.
	 * Triggered when a user declines the membership screening questionnaire for a guild.
	 *
	 * @param _guild_join_request_delete User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_join_request_delete_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_join_request_delete(std::function<void(const guild_join_request_delete_t& _event)> _guild_join_request_delete);
	/**
	 * @brief Detach listener from on_guild_join_request_delete event
	 * 
	 * @param _guild_join_request_delete Handle to remove from event, previously returned by dpp::cluster::on_guild_join_request_delete()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_join_request_delete(const event_handle _guild_join_request_delete);

	/**
	 * @brief Called when a new interaction is created.
	 * Interactions are created by discord when commands you have registered are issued
	 * by a user. For an example of this in action please see \ref slashcommands
	 *
	 * @param _interaction_create  User function to attach to event
	 * Event is called with the parameter type `const` dpp::interaction_create_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_interaction_create (std::function<void(const interaction_create_t& _event)> _interaction_create);
	/**
	 * @brief Detach listener from on_interaction_create event
	 * 
	 * @param _interaction_create Handle to remove from event, previously returned by dpp::cluster::on_interaction_create()
	 * @return true on successful detach of listener
	 */
	bool detach_interaction_create(const event_handle _interaction_create);

	/**
	 * @brief Called when a button is clicked attached to a message.
	 * Button clicks are triggered by discord when buttons are clicked which you have
	 * associated with a message using dpp::component.
	 *
	 * @param _button_click  User function to attach to event
	 * Event is called with the parameter type `const` dpp::button_click_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_button_click (std::function<void(const button_click_t& _event)> _button_click);
	/**
	 * @brief Detach listener from on_button_click event
	 * 
	 * @param _button_click Handle to remove from event, previously returned by dpp::cluster::on_button_click()
	 * @return true on successful detach of listener
	 */
	bool detach_button_click(const event_handle _button_click);

	/**
	 * @brief Called when an auto completed field needs suggestions to present to the user
	 * This is triggered by discord when option choices have auto completion enabled which you have
	 * associated with a dpp::slashcommand.
	 *
	 * @param _autocomplete  User function to attach to event
	 * Event is called with the parameter type `const` dpp::autocomplete_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_autocomplete (std::function<void(const autocomplete_t& _event)> _autocomplete);
	/**
	 * @brief Detach listener from on_autocomplete event
	 * 
	 * @param _autocomplete Handle to remove from event, previously returned by dpp::cluster::on_autocomplete()
	 * @return true on successful detach of listener
	 */
	bool detach_autocomplete(const event_handle _autocomplete);

	/**
	 * @brief Called when a select menu is clicked attached to a message.
	 * Select menu clicks are triggered by discord when select menus are clicked which you have
	 * associated with a message using dpp::component.
	 *
	 * @param _select_click  User function to attach to event
	 * Event is called with the parameter type `const` dpp::select_click_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_select_click (std::function<void(const select_click_t& _event)> _select_click);
	/**
	 * @brief Detach listener from on_select_click event
	 * 
	 * @param _select_click Handle to remove from event, previously returned by dpp::cluster::on_select_click()
	 * @return true on successful detach of listener
	 */
	bool detach_select_click(const event_handle _select_click);

	/**
	 * @brief Called when a guild is deleted.
	 * A guild can be deleted via the bot being kicked, the bot leaving the guild
	 * explicitly with dpp::guild_delete, or via the guild being unavaialble due to
	 * an outage.
	 *
	 * @param _guild_delete  User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_delete_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_delete (std::function<void(const guild_delete_t& _event)> _guild_delete);
	/**
	 * @brief Detach listener from on_guild_delete event
	 * 
	 * @param _guild_delete Handle to remove from event, previously returned by dpp::cluster::on_guild_delete()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_delete(const event_handle _guild_delete);

	/**
	 * @brief Called when a channel is deleted from a guild.
	 * The channel will still be temporarily avaialble in the cache. Pointers to the
	 * channel should not be retained long-term as they will be deleted by the garbage
	 * collector.
	 *
	 * @param _channel_delete  User function to attach to event
	 * Event is called with the parameter type `const` dpp::channel_delete_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_channel_delete (std::function<void(const channel_delete_t& _event)> _channel_delete);
	/**
	 * @brief Detach listener from on_channel_delete event
	 * 
	 * @param _channel_delete Handle to remove from event, previously returned by dpp::cluster::on_channel_delete()
	 * @return true on successful detach of listener
	 */
	bool detach_channel_delete(const event_handle _channel_delete);

	/**
	 * @brief Called when a channel is edited on a guild.
	 * The new channel details have already been applied to the guild when you
	 * receive this event.
	 *
	 * @param _channel_update  User function to attach to event
	 * Event is called with the parameter type `const` dpp::channel_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_channel_update (std::function<void(const channel_update_t& _event)> _channel_update);
	/**
	 * @brief Detach listener from on_channel_update event
	 * 
	 * @param _channel_update Handle to remove from event, previously returned by dpp::cluster::on_channel_update()
	 * @return true on successful detach of listener
	 */
	bool detach_channel_update(const event_handle _channel_update);

	/**
	 * @brief Called when a shard is connected and ready.
	 * A set of on_guild_create events will follow this event.
	 *
	 * @param _ready  User function to attach to event
	 * Event is called with the parameter type `const` dpp::ready_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_ready (std::function<void(const ready_t& _event)> _ready);
	/**
	 * @brief Detach listener from on_ready event
	 * 
	 * @param _ready Handle to remove from event, previously returned by dpp::cluster::on_ready()
	 * @return true on successful detach of listener
	 */
	bool detach_ready(const event_handle _ready);

	/**
	 * @brief Called when a message is deleted.
	 * The message has already been deleted from Discord when you
	 * receive this event.
	 *
	 * @param _message_delete  User function to attach to event
	 * Event is called with the parameter type `const` dpp::message_delete_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_message_delete (std::function<void(const message_delete_t& _event)> _message_delete);
	/**
	 * @brief Detach listener from on_message_delete event
	 * 
	 * @param _message_delete Handle to remove from event, previously returned by dpp::cluster::on_message_delete()
	 * @return true on successful detach of listener
	 */
	bool detach_message_delete(const event_handle _message_delete);

	/**
	 * @brief Called when an application command (slash command) is deleted.
	 *
	 * @param _application_command_delete  User function to attach to event
	 * Event is called with the parameter type `const` dpp::application_command_delete_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_application_command_delete (std::function<void(const application_command_delete_t& _event)> _application_command_delete);
	/**
	 * @brief Detach listener from on_application_command_delete event
	 * 
	 * @param _application_command_delete Handle to remove from event, previously returned by dpp::cluster::on_application_command_delete()
	 * @return true on successful detach of listener
	 */
	bool detach_application_command_delete(const event_handle _application_command_delete);

	/**
	 * @brief Called when a user leaves a guild (either through being kicked, or choosing to leave)
	 *
	 * @param _guild_member_remove  User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_member_remove_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_member_remove (std::function<void(const guild_member_remove_t& _event)> _guild_member_remove);
	/**
	 * @brief Detach listener from on_guild_member_remove event
	 * 
	 * @param _guild_member_remove Handle to remove from event, previously returned by dpp::cluster::on_guild_member_remove()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_member_remove(const event_handle _guild_member_remove);

	/**
	 * @brief Called when a new application command (slash command) is registered.
	 *
	 * @param _application_command_create  User function to attach to event
	 * Event is called with the parameter type `const` dpp::application_command_create_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_application_command_create (std::function<void(const application_command_create_t& _event)> _application_command_create);
	/**
	 * @brief Detach listener from on_application_command_create event
	 * 
	 * @param _application_command_create Handle to remove from event, previously returned by dpp::cluster::on_application_command_create()
	 * @return true on successful detach of listener
	 */
	bool detach_application_command_create(const event_handle _application_command_create);

	/**
	 * @brief Called when a connection to a shard successfully resumes.
	 * A resumed session does not need to re-synchronise guilds, members, etc.
	 * This is generally non-fatal and informational only.
	 *
	 * @param _resumed  User function to attach to event
	 * Event is called with the parameter type `const` dpp::resumed_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_resumed (std::function<void(const resumed_t& _event)> _resumed);
	/**
	 * @brief Detach listener from on_resumed event
	 * 
	 * @param _resumed Handle to remove from event, previously returned by dpp::cluster::on_resumed()
	 * @return true on successful detach of listener
	 */
	bool detach_resumed(const event_handle _resumed);

	/**
	 * @brief Called when a new role is created on a guild.
	 *
	 * @param _guild_role_create  User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_role_create_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_role_create (std::function<void(const guild_role_create_t& _event)> _guild_role_create);
	/**
	 * @brief Detach listener from on_guild_role_create event
	 * 
	 * @param _guild_role_create Handle to remove from event, previously returned by dpp::cluster::on_guild_role_create()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_role_create(const event_handle _guild_role_create);

	/**
	 * @brief Called when a user is typing on a channel.
	 *
	 * @param _typing_start  User function to attach to event
	 * Event is called with the parameter type `const` dpp::typing_start_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_typing_start (std::function<void(const typing_start_t& _event)> _typing_start);
	/**
	 * @brief Detach listener from on_typing_start event
	 * 
	 * @param _typing_start Handle to remove from event, previously returned by dpp::cluster::on_typing_start()
	 * @return true on successful detach of listener
	 */
	bool detach_typing_start(const event_handle _typing_start);

	/**
	 * @brief Called when a new reaction is added to a message.
	 *
	 * @param _message_reaction_add  User function to attach to event
	 * Event is called with the parameter type `const` dpp::message_reaction_add_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_message_reaction_add (std::function<void(const message_reaction_add_t& _event)> _message_reaction_add);
	/**
	 * @brief Detach listener from on_message_reaction_add event
	 * 
	 * @param _message_reaction_add Handle to remove from event, previously returned by dpp::cluster::on_message_reaction_add()
	 * @return true on successful detach of listener
	 */
	bool detach_message_reaction_add(const event_handle _message_reaction_add);

	/**
	 * @brief Called when a set of members is received for a guild.
	 * D++ will request these for all new guilds if needed, after the on_guild_create
	 * events.
	 *
	 * @param _guild_members_chunk  User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_members_chunk_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_members_chunk (std::function<void(const guild_members_chunk_t& _event)> _guild_members_chunk);
	/**
	 * @brief Detach listener from on_guild_members_chunk event
	 * 
	 * @param _guild_members_chunk Handle to remove from event, previously returned by dpp::cluster::on_guild_members_chunk()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_members_chunk(const event_handle _guild_members_chunk);

	/**
	 * @brief Called when a single reaction is removed from a message.
	 *
	 * @param _message_reaction_remove  User function to attach to event
	 * Event is called with the parameter type `const` dpp::message_reaction_remove_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_message_reaction_remove (std::function<void(const message_reaction_remove_t& _event)> _message_reaction_remove);
	/**
	 * @brief Detach listener from on_message_reaction_remove event
	 * 
	 * @param _message_reaction_remove Handle to remove from event, previously returned by dpp::cluster::on_message_reaction_remove()
	 * @return true on successful detach of listener
	 */
	bool detach_message_reaction_remove(const event_handle _message_reaction_remove);

	/**
	 * @brief Called when a new guild is created.
	 * D++ will request members for the guild for its cache using guild_members_chunk.
	 *
	 * @param _guild_create  User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_create_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_create (std::function<void(const guild_create_t& _event)> _guild_create);
	/**
	 * @brief Detach listener from on_guild_create event
	 * 
	 * @param _guild_create Handle to remove from event, previously returned by dpp::cluster::on_guild_create()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_create(const event_handle _guild_create);

	/**
	 * @brief Called when a new channel is created on a guild.
	 *
	 * @param _channel_create  User function to attach to event
	 * Event is called with the parameter type `const` dpp::channel_create_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_channel_create (std::function<void(const channel_create_t& _event)> _channel_create);
	/**
	 * @brief Detach listener from on_channel_create event
	 * 
	 * @param _channel_create Handle to remove from event, previously returned by dpp::cluster::on_channel_create()
	 * @return true on successful detach of listener
	 */
	bool detach_channel_create(const event_handle _channel_create);

	/**
	 * @brief Called when all reactions for a particular emoji are removed from a message.
	 *
	 * @param _message_reaction_remove_emoji  User function to attach to event
	 * Event is called with the parameter type `const` dpp::message_reaction_remove_emoji_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_message_reaction_remove_emoji (std::function<void(const message_reaction_remove_emoji_t& _event)> _message_reaction_remove_emoji);
	/**
	 * @brief Detach listener from on_message_reaction_remove_emoji event
	 * 
	 * @param _message_reaction_remove_emoji Handle to remove from event, previously returned by dpp::cluster::on_message_reaction_remove_emoji()
	 * @return true on successful detach of listener
	 */
	bool detach_message_reaction_remove_emoji(const event_handle _message_reaction_remove_emoji);

	/**
	 * @brief Called when multiple messages are deleted from a channel or DM.
	 *
	 * @param _message_delete_bulk  User function to attach to event
	 * Event is called with the parameter type `const` dpp::message_delete_bulk_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_message_delete_bulk (std::function<void(const message_delete_bulk_t& _event)> _message_delete_bulk);
	/**
	 * @brief Detach listener from on_message_delete_bulk event
	 * 
	 * @param _message_delete_bulk Handle to remove from event, previously returned by dpp::cluster::on_message_delete_bulk()
	 * @return true on successful detach of listener
	 */
	bool detach_message_delete_bulk(const event_handle _message_delete_bulk);

	/**
	 * @brief Called when an existing role is updated on a guild.
	 *
	 * @param _guild_role_update  User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_role_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_role_update (std::function<void(const guild_role_update_t& _event)> _guild_role_update);
	/**
	 * @brief Detach listener from on_guild_role_update event
	 * 
	 * @param _guild_role_update Handle to remove from event, previously returned by dpp::cluster::on_guild_role_update()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_role_update(const event_handle _guild_role_update);

	/**
	 * @brief Called when a role is deleted in a guild.
	 *
	 * @param _guild_role_delete  User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_role_delete_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_role_delete (std::function<void(const guild_role_delete_t& _event)> _guild_role_delete);
	/**
	 * @brief Detach listener from on_guild_role_delete event
	 * 
	 * @param _guild_role_delete Handle to remove from event, previously returned by dpp::cluster::on_guild_role_delete()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_role_delete(const event_handle _guild_role_delete);

	/**
	 * @brief Called when a message is pinned.
	 * Note that the pinned message is not returned to this event, just the timestamp
	 * of the last pinned message.
	 *
	 * @param _channel_pins_update  User function to attach to event
	 * Event is called with the parameter type `const` dpp::channel_pins_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_channel_pins_update (std::function<void(const channel_pins_update_t& _event)> _channel_pins_update);
	/**
	 * @brief Detach listener from on_channel_pins_update event
	 * 
	 * @param _channel_pins_update Handle to remove from event, previously returned by dpp::cluster::on_channel_pins_update()
	 * @return true on successful detach of listener
	 */
	bool detach_channel_pins_update(const event_handle _channel_pins_update);

	/**
	 * @brief Called when all reactions are removed from a message.
	 *
	 * @param _message_reaction_remove_all  User function to attach to event
	 * Event is called with the parameter type `const` dpp::message_reaction_remove_all_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_message_reaction_remove_all (std::function<void(const message_reaction_remove_all_t& _event)> _message_reaction_remove_all);
	/**
	 * @brief Detach listener from on_message_reaction_remove_all event
	 * 
	 * @param _message_reaction_remove_all Handle to remove from event, previously returned by dpp::cluster::on_message_reaction_remove_all()
	 * @return true on successful detach of listener
	 */
	bool detach_message_reaction_remove_all(const event_handle _message_reaction_remove_all);

	/**
	 * @brief Called when we are told which voice server we can use.
	 * This will be sent either when we establish a new voice channel connection,
	 * or as discord rearrange their infrastructure.
	 *
	 * @param _voice_server_update  User function to attach to event
	 * Event is called with the parameter type `const` dpp::voice_server_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_voice_server_update (std::function<void(const voice_server_update_t& _event)> _voice_server_update);
	/**
	 * @brief Detach listener from on_voice_server_update event
	 * 
	 * @param _voice_server_update Handle to remove from event, previously returned by dpp::cluster::on_voice_server_update()
	 * @return true on successful detach of listener
	 */
	bool detach_voice_server_update(const event_handle _voice_server_update);

	/**
	 * @brief Called when new emojis are added to a guild.
	 * The complete set of emojis is sent every time.
	 *
	 * @param _guild_emojis_update  User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_emojis_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_emojis_update (std::function<void(const guild_emojis_update_t& _event)> _guild_emojis_update);
	/**
	 * @brief Detach listener from on_guild_emojis_update event
	 * 
	 * @param _guild_emojis_update Handle to remove from event, previously returned by dpp::cluster::on_guild_emojis_update()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_emojis_update(const event_handle _guild_emojis_update);

	/**
	 * @brief Called when new stickers are added to a guild.
	 * The complete set of stickers is sent every time.
	 *
	 * @param _guild_stickers_update  User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_stickers_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_stickers_update (std::function<void(const guild_stickers_update_t& _event)> _guild_stickers_update);
	/**
	 * @brief Detach listener from on_guild_stickers_update event
	 * 
	 * @param _guild_stickers_update Handle to remove from event, previously returned by dpp::cluster::on_guild_stickers_update()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_stickers_update(const event_handle _guild_stickers_update);

	/**
	 * @brief Called when a user's presence is updated.
	 * To receive these you will need the GUILD_PRESENCES privileged intent.
	 * You will receive many of these, very often, and receiving them will significantly
	 * increase your bot's CPU usage. If you don't need them it is recommended to not ask
	 * for them.
	 *
	 * @param _presence_update  User function to attach to event
	 * Event is called with the parameter type `const` dpp::presence_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_presence_update (std::function<void(const presence_update_t& _event)> _presence_update);
	/**
	 * @brief Detach listener from on_presence_update event
	 * 
	 * @param _presence_update Handle to remove from event, previously returned by dpp::cluster::on_presence_update()
	 * @return true on successful detach of listener
	 */
	bool detach_presence_update(const event_handle _presence_update);

	/**
	 * @brief Called when the webhooks for a guild are updated.
	 *
	 * @param _webhooks_update  User function to attach to event
	 * Event is called with the parameter type `const` dpp::webhooks_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_webhooks_update (std::function<void(const webhooks_update_t& _event)> _webhooks_update);
	/**
	 * @brief Detach listener from on_webhooks_update event
	 * 
	 * @param _webhooks_update Handle to remove from event, previously returned by dpp::cluster::on_webhooks_update()
	 * @return true on successful detach of listener
	 */
	bool detach_webhooks_update(const event_handle _webhooks_update);

	/**
	 * @brief Called when a new member joins a guild.
	 *
	 * @param _guild_member_add  User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_member_add_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_member_add (std::function<void(const guild_member_add_t& _event)> _guild_member_add);
	/**
	 * @brief Detach listener from on_guild_member_add event
	 * 
	 * @param _guild_member_add Handle to remove from event, previously returned by dpp::cluster::on_guild_member_add()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_member_add(const event_handle _guild_member_add);

	/**
	 * @brief Called when an invite is deleted from a guild.
	 *
	 * @param _invite_delete  User function to attach to event
	 * Event is called with the parameter type `const` dpp::invite_delete_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_invite_delete (std::function<void(const invite_delete_t& _event)> _invite_delete);
	/**
	 * @brief Detach listener from on_invite_delete event
	 * 
	 * @param _invite_delete Handle to remove from event, previously returned by dpp::cluster::on_invite_delete()
	 * @return true on successful detach of listener
	 */
	bool detach_invite_delete(const event_handle _invite_delete);

	/**
	 * @brief Called when details of a guild are updated.
	 *
	 * @param _guild_update  User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_update (std::function<void(const guild_update_t& _event)> _guild_update);
	/**
	 * @brief Detach listener from on_guild_update event
	 * 
	 * @param _guild_update Handle to remove from event, previously returned by dpp::cluster::on_guild_update()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_update(const event_handle _guild_update);

	/**
	 * @brief Called when an integration is updated for a guild.
	 * This returns the complete list.
	 * An integration is a connection to a guild of a user's associated accounts,
	 * e.g. youtube or twitch, for automatic assignment of roles etc.
	 *
	 * @param _guild_integrations_update  User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_integrations_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_integrations_update (std::function<void(const guild_integrations_update_t& _event)> _guild_integrations_update);
	/**
	 * @brief Detach listener from on_guild_integrations_update event
	 * 
	 * @param _guild_integrations_update Handle to remove from event, previously returned by dpp::cluster::on_guild_integrations_update()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_integrations_update(const event_handle _guild_integrations_update);

	/**
	 * @brief Called when details of a guild member (e.g. their roles or nickname) are updated.
	 *
	 * @param _guild_member_update  User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_member_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_member_update (std::function<void(const guild_member_update_t& _event)> _guild_member_update);
	/**
	 * @brief Detach listener from on_guild_member_update event
	 * 
	 * @param _guild_member_update Handle to remove from event, previously returned by dpp::cluster::on_guild_member_update()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_member_update(const event_handle _guild_member_update);

	/**
	 * @brief Called when an application command (slash command) is updated.
	 * You will only receive this event for application commands that belong to your bot/application.
	 *
	 * @param _application_command_update  User function to attach to event
	 * Event is called with the parameter type `const` dpp::application_command_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_application_command_update (std::function<void(const application_command_update_t& _event)> _application_command_update);
	/**
	 * @brief Detach listener from on_application_command_update event
	 * 
	 * @param _application_command_update Handle to remove from event, previously returned by dpp::cluster::on_application_command_update()
	 * @return true on successful detach of listener
	 */
	bool detach_application_command_update(const event_handle _application_command_update);

	/**
	 * @brief Called when a new invite is created for a guild.
	 *
	 * @param _invite_create  User function to attach to event
	 * Event is called with the parameter type `const` dpp::invite_create_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_invite_create (std::function<void(const invite_create_t& _event)> _invite_create);
	/**
	 * @brief Detach listener from on_invite_create event
	 * 
	 * @param _invite_create Handle to remove from event, previously returned by dpp::cluster::on_invite_create()
	 * @return true on successful detach of listener
	 */
	bool detach_invite_create(const event_handle _invite_create);

	/**
	 * @brief Called when a message is updated (edited).
	 *
	 * @param _message_update  User function to attach to event
	 * Event is called with the parameter type `const` dpp::message_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_message_update (std::function<void(const message_update_t& _event)> _message_update);
	/**
	 * @brief Detach listener from on_message_update event
	 * 
	 * @param _message_update Handle to remove from event, previously returned by dpp::cluster::on_message_update()
	 * @return true on successful detach of listener
	 */
	bool detach_message_update(const event_handle _message_update);

	/**
	 * @brief Called when a user is updated.
	 * This is separate to guild_member_update and includes things such as an avatar change,
	 * username change, discriminator change or change in subscription status for nitro.
	 *
	 * @param _user_update  User function to attach to event
	 * Event is called with the parameter type `const` dpp::user_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_user_update (std::function<void(const user_update_t& _event)> _user_update);
	/**
	 * @brief Detach listener from on_user_update event
	 * 
	 * @param _user_update Handle to remove from event, previously returned by dpp::cluster::on_user_update()
	 * @return true on successful detach of listener
	 */
	bool detach_user_update(const event_handle _user_update);

	/**
	 * @brief Called when a new message arrives from discord.
	 * Note that D++ does not cache messages. If you want to cache these objects you
	 * should create something yourself within your bot. Caching of messages is not on
	 * the roadmap to be supported as it consumes excessive amounts of RAM.
	 *
	 * @param _message_create  User function to attach to event
	 * Event is called with the parameter type `const` dpp::message_create_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_message_create (std::function<void(const message_create_t& _event)> _message_create);
	/**
	 * @brief Detach listener from on_message_create event
	 * 
	 * @param _message_create Handle to remove from event, previously returned by dpp::cluster::on_message_create()
	 * @return true on successful detach of listener
	 */
	bool detach_message_create(const event_handle _message_create);

	/**
	 * @brief Called when a ban is added to a guild.
	 *
	 * @param _guild_ban_add  User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_ban_add_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_ban_add (std::function<void(const guild_ban_add_t& _event)> _guild_ban_add);
	/**
	 * @brief Detach listener from on_guild_ban_add event
	 * 
	 * @param _guild_ban_add Handle to remove from event, previously returned by dpp::cluster::on_guild_ban_add()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_ban_add(const event_handle _guild_ban_add);

	/**
	 * @brief Called when a ban is removed from a guild.
	 *
	 * @param _guild_ban_remove  User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_ban_remove_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_ban_remove (std::function<void(const guild_ban_remove_t& _event)> _guild_ban_remove);
	/**
	 * @brief Detach listener from on_guild_ban_remove event
	 * 
	 * @param _guild_ban_remove Handle to remove from event, previously returned by dpp::cluster::on_guild_ban_remove()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_ban_remove(const event_handle _guild_ban_remove);

	/**
	 * @brief Called when a new intgration is attached to a guild by a user.
	 * An integration is a connection to a guild of a user's associated accounts,
	 * e.g. youtube or twitch, for automatic assignment of roles etc.
	 *
	 * @param _integration_create User function to attach to event
	 * Event is called with the parameter type `const` dpp::integration_create_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_integration_create (std::function<void(const integration_create_t& _event)> _integration_create);
	/**
	 * @brief Detach listener from on_integration_create event
	 * 
	 * @param _integration_create Handle to remove from event, previously returned by dpp::cluster::on_integration_create()
	 * @return true on successful detach of listener
	 */
	bool detach_integration_create(const event_handle _integration_create);

	/**
	 * @brief Called when an integration is updated by a user.
	 * This returns details of just the single integration that has changed.
	 * An integration is a connection to a guild of a user's associated accounts,
	 * e.g. youtube or twitch, for automatic assignment of roles etc.
	 *
	 * @param _integration_update User function to attach to event
	 * Event is called with the parameter type `const` dpp::integration_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_integration_update (std::function<void(const integration_update_t& _event)> _integration_update);
	/**
	 * @brief Detach listener from on_integration_update event
	 * 
	 * @param _integration_update Handle to remove from event, previously returned by dpp::cluster::on_integration_update()
	 * @return true on successful detach of listener
	 */
	bool detach_integration_update(const event_handle _integration_update);

	/**
	 * @brief Called when an integration is removed by a user.
	 * An integration is a connection to a guild of a user's associated accounts,
	 * e.g. youtube or twitch, for automatic assignment of roles etc.
	 *
	 * @param _integration_delete User function to attach to event
	 * Event is called with the parameter type `const` dpp::integration_delete_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_integration_delete (std::function<void(const integration_delete_t& _event)> _integration_delete);
	/**
	 * @brief Detach listener from on_integration_delete event
	 * 
	 * @param _integration_delete Handle to remove from event, previously returned by dpp::cluster::on_integration_delete()
	 * @return true on successful detach of listener
	 */
	bool detach_integration_delete(const event_handle _integration_delete);

	/**
	 * @brief Called when a thread is created
	 * Note: Threads are not cached by D++, but a list of thread IDs is accessible in a guild object
	 *
	 * @param _thread_create User function to attach to event
	 * Event is called with the parameter type `const` dpp::thread_create_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_thread_create (std::function<void(const thread_create_t& _event)> _thread_create);
	/**
	 * @brief Detach listener from on_thread_create event
	 * 
	 * @param _thread_create Handle to remove from event, previously returned by dpp::cluster::on_thread_create()
	 * @return true on successful detach of listener
	 */
	bool detach_thread_create(const event_handle _thread_create);

	/**
	 * @brief Called when a thread is updated
	 *
	 * @param _thread_update User function to attach to event
	 * Event is called with the parameter type `const` dpp::thread_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_thread_update (std::function<void(const thread_update_t& _event)> _thread_update);
	/**
	 * @brief Detach listener from on_thread_update event
	 * 
	 * @param _thread_update Handle to remove from event, previously returned by dpp::cluster::on_thread_update()
	 * @return true on successful detach of listener
	 */
	bool detach_thread_update(const event_handle _thread_update);

	/**
	 * @brief Called when a thread is deleted
	 *
	 * @param _thread_delete User function to attach to event
	 * Event is called with the parameter type `const` dpp::thread_delete_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_thread_delete (std::function<void(const thread_delete_t& _event)> _thread_delete);
	/**
	 * @brief Detach listener from on_thread_delete event
	 * 
	 * @param _thread_delete Handle to remove from event, previously returned by dpp::cluster::on_thread_delete()
	 * @return true on successful detach of listener
	 */
	bool detach_thread_delete(const event_handle _thread_delete);

	/**
	 * @brief Called when thread list is synced (upon gaining access to a channel)
	 * Note: Threads are not cached by D++, but a list of thread IDs is accessible in a guild object
	 *
	 * @param _thread_list_sync User function to attach to event
	 * Event is called with the parameter type `const` dpp::thread_list_sync_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_thread_list_sync (std::function<void(const thread_list_sync_t& _event)> _thread_list_sync);
	/**
	 * @brief Detach listener from on_thread_list_sync event
	 * 
	 * @param _thread_list_sync Handle to remove from event, previously returned by dpp::cluster::on_thread_list_sync()
	 * @return true on successful detach of listener
	 */
	bool detach_thread_list_sync(const event_handle _thread_list_sync);

	/**
	 * @brief Called when current user's thread member object is updated
	 *
	 * @param _thread_member_update User function to attach to event
	 * Event is called with the parameter type `const` dpp::thread_member_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_thread_member_update (std::function<void(const thread_member_update_t& _event)> _thread_member_update);
	/**
	 * @brief Detach listener from on_thread_member_update event
	 * 
	 * @param _thread_member_update Handle to remove from event, previously returned by dpp::cluster::on_thread_member_update()
	 * @return true on successful detach of listener
	 */
	bool detach_thread_member_update(const event_handle _thread_member_update);

	/**
	 * @brief Called when a thread's member list is updated (without GUILD_MEMBERS intent, is only called for current user)
	 *
	 * @param _thread_members_update User function to attach to event
	 * Event is called with the parameter type `const` dpp::thread_members_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_thread_members_update (std::function<void(const thread_members_update_t& _event)> _thread_members_update);
	/**
	 * @brief Detach listener from on_thread_members_update event
	 * 
	 * @param _thread_members_update Handle to remove from event, previously returned by dpp::cluster::on_thread_members_update()
	 * @return true on successful detach of listener
	 */
	bool detach_thread_members_update(const event_handle _thread_members_update);

	/**
	 * @brief Called when a new scheduled event is created
	 *
	 * @param _guild_scheduled_event_create User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_scheduled_event_create_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_scheduled_event_create (std::function<void(const guild_scheduled_event_create_t& _event)> _guild_scheduled_event_create);
	/**
	 * @brief Detach listener from on_guild_scheduled_event_create
	 * 
	 * @param _guild_scheduled_event_create Handle to remove from event, previously returned by dpp::cluster::on_guild_scheduled_event_create()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_scheduled_event_create(const event_handle _guild_scheduled_event_create);

	/**
	 * @brief Called when a new scheduled event is updated
	 *
	 * @param _guild_scheduled_event_update User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_scheduled_event_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_scheduled_event_update (std::function<void(const guild_scheduled_event_update_t& _event)> _guild_scheduled_event_update);
	/**
	 * @brief Detach listener from on_guild_scheduled_event_update
	 * 
	 * @param _guild_scheduled_event_update Handle to remove from event, previously returned by dpp::cluster::on_guild_scheduled_event_update()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_scheduled_event_update(const event_handle _guild_scheduled_event_update);

	/**
	 * @brief Called when a new scheduled event is deleted
	 *
	 * @param _guild_scheduled_event_delete User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_scheduled_event_delete_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_scheduled_event_delete (std::function<void(const guild_scheduled_event_delete_t& _event)> _guild_scheduled_event_delete);
	/**
	 * @brief Detach listener from on_guild_scheduled_event_delete
	 * 
	 * @param _guild_scheduled_event_delete Handle to remove from event, previously returned by dpp::cluster::on_guild_scheduled_event_delete()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_scheduled_event_delete(const event_handle _guild_scheduled_event_delete);

	/**
	 * @brief Called when a user is added to a scheduled event
	 *
	 * @param _guild_scheduled_event_user_add User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_scheduled_event_user_add_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_scheduled_event_user_add (std::function<void(const guild_scheduled_event_user_add_t& _event)> _guild_scheduled_event_user_add);
	/**
	 * @brief Detach listener from on_guild_scheduled_event_user_add
	 * 
	 * @param _guild_scheduled_event_user_add Handle to remove from event, previously returned by dpp::cluster::on_guild_scheduled_event_user_add()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_scheduled_event_user_add(const event_handle _guild_scheduled_event_user_add);

	/**
	 * @brief Called when a user is removed to a scheduled event
	 *
	 * @param _guild_scheduled_event_user_remove User function to attach to event
	 * Event is called with the parameter type `const` dpp::guild_scheduled_event_user_remove_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_guild_scheduled_event_user_remove (std::function<void(const guild_scheduled_event_user_remove_t& _event)> _guild_scheduled_event_user_remove);
	/**
	 * @brief Detach listener from on_guild_scheduled_event_user_remove
	 * 
	 * @param _guild_scheduled_event_user_remove Handle to remove from event, previously returned by dpp::cluster::on_guild_scheduled_event_user_remove()
	 * @return true on successful detach of listener
	 */
	bool detach_guild_scheduled_event_user_remove(const event_handle _guild_scheduled_event_user_remove);

	/**
	 * @brief Called when packets are sent from the voice buffer.
	 * The voice buffer contains packets that are already encoded with Opus and encrypted
	 * with Sodium, and merged into packets by the repacketizer, which is done in the
	 * dpp::discord_voice_client::send_audio method. You should use the buffer size properties
	 * of dpp::voice_buffer_send_t to determine if you should fill the buffer with more
	 * content.
	 *
	 * @param _voice_buffer_send User function to attach to event
	 * Event is called with the parameter type `const` dpp::voice_buffer_send_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_voice_buffer_send (std::function<void(const voice_buffer_send_t& _event)> _voice_buffer_send);
	/**
	 * @brief Detach listener from on_voice_buffer_send event
	 * 
	 * @param _voice_buffer_send Handle to remove from event, previously returned by dpp::cluster::on_voice_buffer_send()
	 * @return true on successful detach of listener
	 */
	bool detach_voice_buffer_send(const event_handle _voice_buffer_send);

	/**
	 * @brief Called when a user is talking on a voice channel.
	 *
	 * @param _voice_user_talking User function to attach to event
	 * Event is called with the parameter type `const` dpp::voice_user_talking_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_voice_user_talking (std::function<void(const voice_user_talking_t& _event)> _voice_user_talking);
	/**
	 * @brief Detach listener from on_voice_user_talking event
	 * 
	 * @param _voice_user_talking Handle to remove from event, previously returned by dpp::cluster::on_voice_user_talking()
	 * @return true on successful detach of listener
	 */
	bool detach_voice_user_talking(const event_handle _voice_user_talking);

	/**
	 * @brief Called when a voice channel is connected and ready to send audio.
	 * Note that this is not directly attached to the READY event of the websocket,
	 * as there is further connection that needs to be done before audio is ready to send.
	 *
	 * @param _voice_ready User function to attach to event
	 * Event is called with the parameter type `const` dpp::voice_ready_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_voice_ready (std::function<void(const voice_ready_t& _event)> _voice_ready);
	/**
	 * @brief Detach listener from on_voice_ready event
	 * 
	 * @param _voice_ready Handle to remove from event, previously returned by dpp::cluster::on_voice_ready()
	 * @return true on successful detach of listener
	 */
	bool detach_voice_ready(const event_handle _voice_ready);

	/**
	 * @brief Called when new audio data is received.
	 * Each separate user's audio from the voice channel will arrive tagged with
	 * their user id in the event, if a user can be attributed to the received audio.
	 * 
	 * @note Receiveing audio for bots is not officially supported by discord.
	 * 
	 * @param _voice_receive User function to attach to event
	 * Event is called with the parameter type `const` dpp::voice_receive_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_voice_receive (std::function<void(const voice_receive_t& _event)> _voice_receive);
	/**
	 * @brief Detach listener from on_voice_receive event
	 * 
	 * @param _voice_receive Handle to remove from event, previously returned by dpp::cluster::on_voice_receive()
	 * @return true on successful detach of listener
	 */
	bool detach_voice_receive(const event_handle _voice_receive);

	/**
	 * @brief Called when sending of audio passes over a track marker.
	 * Track markers are arbitrarily placed "bookmarks" in the audio buffer, placed
	 * by the bot developer. Each track marker can have a string value associated with it
	 * which is specified in dpp::discord_voice_client::insert_marker and returned to this
	 * event.
	 *
	 * @param _voice_track_marker User function to attach to event
	 * Event is called with the parameter type `const` dpp::voice_track_marker_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_voice_track_marker (std::function<void(const voice_track_marker_t& _event)> _voice_track_marker);
	/**
	 * @brief Detach listener from on_voice_track_marker event
	 * 
	 * @param _voice_track_marker Handle to remove from event, previously returned by dpp::cluster::on_voice_track_marker()
	 * @return true on successful detach of listener
	 */
	bool detach_voice_track_marker(const event_handle _voice_track_marker);

	/**
	 * @brief Called when a new stage instance is created on a stage channel.
	 *
	 * @param _stage_instance_create User function to attach to event
	 * 
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_stage_instance_create (std::function<void(const stage_instance_create_t& _event)> _stage_instance_create);
	/**
	 * @brief Detach listener from on_stage_instance_create event
	 * 
	 * @param _stage_instance_create Handle to remove from event, previously returned by dpp::cluster::on_stage_instance_create()
	 * @return true on successful detach of listener
	 */
	bool detach_stage_instance_create(const event_handle _stage_instance_create);

	/**
	 * @brief Called when a stage instance is updated.
	 *
	 * @param _stage_instance_update User function to attach to event
	 * Event is called with the parameter type `const` dpp::stage_instance_update_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_stage_instance_update (std::function<void(const stage_instance_update_t& _event)> _stage_instance_update);
	/**
	 * @brief Detach listener from on_stage_instance_update event
	 * 
	 * @param _stage_instance_update Handle to remove from event, previously returned by dpp::cluster::on_stage_instance_update()
	 * @return true on successful detach of listener
	 */
	bool detach_stage_instance_update(const event_handle _stage_instance_update);

	/**
	 * @brief Called when an existing stage instance is deleted from a stage channel.
	 *
	 * @param _stage_instance_delete User function to attach to event
	 * Event is called with the parameter type `const` dpp::stage_instance_delete_t&
	 * @return event_handle An opaque handle to the attached event, which can be used to refer to it later if needed
	 */
	event_handle on_stage_instance_delete (std::function<void(const stage_instance_delete_t& _event)> _stage_instance_delete);
	/**
	 * @brief Detach listener from on_stage_instance_delete event
	 * 
	 * @param _stage_instance_delete Handle to remove from event, previously returned by dpp::cluster::on_stage_instance_delete()
	 * @return true on successful detach of listener
	 */
	bool detach_stage_instance_delete(const event_handle _stage_instance_delete);

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
	 * @brief Make a HTTP(S) request. For use when wanting asnyncronous access to HTTP APIs outside of Discord.
	 *
	 * @param url Endpoint to post to, e.g. /api/guilds
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
	 * @param interaction_id Interaction id to respond to
	 * @param token Token for the interaction webhook
	 * @param r Response to send
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void interaction_response_create(snowflake interaction_id, const std::string &token, const interaction_response &r, command_completion_event_t callback = {});

	/**
	 * @brief Respond to a slash command
	 *
	 * @param token Token for the interaction webhook
	 * @param r Message to send
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void interaction_response_edit(const std::string &token, const message &r, command_completion_event_t callback = {});

	/**
	 * @brief Create a global slash command (a bot can have a maximum of 100 of these).
	 * 
	 * @note Global commands are cached by discord server-side and can take up to an hour to be visible. For testing,
	 * you should use cluster::guild_command_create instead.
	 *
	 * @param s Slash command to create
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::slashcommmand object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void global_command_create(const slashcommand &s, command_completion_event_t callback = {});

	/**
	 * @brief Get the audit log for a guild
	 *
	 * @param guild_id Guild to get the audit log of
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::auditlog object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_auditlog_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Create a slash command local to a guild
	 *
	 * @param s Slash command to create
	 * @param guild_id Guild ID to create the slash command in
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::slashcommmand object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_command_create(const slashcommand &s, snowflake guild_id, command_completion_event_t callback = {});


	/**
	 * @brief Create/overwrite guild slash commands.
	 * Any existing guild slash commands on this guild will be deleted and replaced with these.
	 *
	 * @param commands Vector of slash commands to create/update.
	 * New guild commands will be available in the guild immediately. If the command did not already exist, it will count toward daily application command create limits.
	 * @param guild_id Guild ID to create/update the slash commands in
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a list of dpp::slashcommmand object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_bulk_command_create(const std::vector<slashcommand> &commands, snowflake guild_id, command_completion_event_t callback = {});

	/**
	 * @brief Create/overwrite global slash commands.
	 * Any existing global slash commands will be deletd and replaced with these.
	 *
	 * @note Global commands are cached by discord server-side and can take up to an hour to be visible. For testing,
	 * you should use cluster::guild_bulk_command_create instead.
	 * 
	 * @param commands Vector of slash commands to create/update.
	 * overwriting existing commands that are registered globally for this application. Updates will be available in all guilds after 1 hour.
	 * Commands that do not already exist will count toward daily application command create limits.
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a list of dpp::slashcommmand object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void global_bulk_command_create(const std::vector<slashcommand> &commands, command_completion_event_t callback = {});

	/**
	 * @brief Edit a global slash command (a bot can have a maximum of 100 of these)
	 *
	 * @param s Slash command to change
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void global_command_edit(const slashcommand &s, command_completion_event_t callback = {});

	/**
	 * @brief Edit a slash command local to a guild
	 *
	 * @param s Slash command to edit
	 * @param guild_id Guild ID to edit the slash command in
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_command_edit(const slashcommand &s, snowflake guild_id, command_completion_event_t callback = {});

	/**
	 * @brief Edit slash command permissions local to a guild,
	 *        permissions are read from s.permissions
	 *
	 * @param s Slash command to edit
	 * @param guild_id Guild ID to edit the slash command in
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_command_edit_permissions(const slashcommand &s, snowflake guild_id, command_completion_event_t callback = {});



	/**
	 * @brief Delete a global slash command (a bot can have a maximum of 100 of these)
	 *
	 * @param id Slash command to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void global_command_delete(snowflake id, command_completion_event_t callback = {});

	/**
	 * @brief Delete a slash command local to a guild
	 *
	 * @param id Slash command to delete
	 * @param guild_id Guild ID to delete the slash command in
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_command_delete(snowflake id, snowflake guild_id, command_completion_event_t callback = {});

	/**
	 * @brief Get the application's slash commands for a guild
	 *
	 * @param guild_id Guild ID to get the slash commands for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::slashcommand_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_commands_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get the application's global slash commands
	 *
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::slashcommand_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void global_commands_get(command_completion_event_t callback);

	/**
	 * @brief Create a direct message, also create the channel for the direct message if needed
	 *
	 * @param user_id User ID of user to send message to
	 * @param m Message object
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void direct_message_create(snowflake user_id, const message &m, command_completion_event_t callback = {});

	/**
	 * @brief Get a message
	 *
	 * @param message_id Message ID
	 * @param channel_id Channel ID
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_get(snowflake message_id, snowflake channel_id, command_completion_event_t callback);

	/**
	 * @brief Get multiple messages
	 *
	 * @param channel_id Channel ID to retrieve messages for
	 * @param around Messages should be retrieved around this ID if this is set to non-zero
	 * @param before Messages before this ID should be retrieved if this is set to non-zero
	 * @param after Messages before this ID should be retrieved if this is set to non-zero
	 * @param limit This number of messages maximum should be returned
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void messages_get(snowflake channel_id, snowflake around, snowflake before, snowflake after, uint8_t limit, command_completion_event_t callback);

	/**
	 * @brief Send a message to a channel. The callback function is called when the message has been sent
	 *
	 * @param m Message to send
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_create(const struct message &m, command_completion_event_t callback = {});

	/**
	 * @brief Crosspost a message. The callback function is called when the message has been sent
	 *
	 * @param message_id Message to crosspost
	 * @param channel_id Channel ID to crosspost from
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_crosspost(snowflake message_id, snowflake channel_id, command_completion_event_t callback = {});

	/**
	 * @brief Edit a message on a channel. The callback function is called when the message has been edited
	 *
	 * @param m Message to edit
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_edit(const struct message &m, command_completion_event_t callback = {});

	/**
	 * @brief Add a reaction to a message. The reaction string must be either an `emojiname:id` or a unicode character.
	 *
	 * @param m Message to add a reaction to
	 * @param reaction Reaction to add. Emojis should be in the form emojiname:id
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_add_reaction(const struct message &m, const std::string &reaction, command_completion_event_t callback = {});

	/**
	 * @brief Delete own reaction from a message. The reaction string must be either an `emojiname:id` or a unicode character.
	 *
	 * @param m Message to delete own reaction from
	 * @param reaction Reaction to delete. The reaction should be in the form emojiname:id
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete_own_reaction(const struct message &m, const std::string &reaction, command_completion_event_t callback = {});

	/**
	 * @brief Delete a user's reaction from a message. The reaction string must be either an `emojiname:id` or a unicode character
	 *
	 * @param m Message to delete a user's reaction from
	 * @param user_id User ID who's reaction you want to remove
	 * @param reaction Reaction to remove. Reactions should be in the form emojiname:id
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete_reaction(const struct message &m, snowflake user_id, const std::string &reaction, command_completion_event_t callback = {});

	/**
	 * @brief Get reactions on a message for a particular emoji. The reaction string must be either an `emojiname:id` or a unicode character
	 *
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
	 * @param m Message to delete reactions from
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete_all_reactions(const struct message &m, command_completion_event_t callback = {});

	/**
	 * @brief Delete all reactions on a message using a particular emoji. The reaction string must be either an `emojiname:id` or a unicode character
	 *
	 * @param m Message to delete reactions from
	 * @param reaction Reaction to delete, in the form emojiname:id or a unicode character
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete_reaction_emoji(const struct message &m, const std::string &reaction, command_completion_event_t callback = {});

	/**
	 * @brief Add a reaction to a message by id. The reaction string must be either an `emojiname:id` or a unicode character.
	 *
	 * @param message_id Message to add reactions to
	 * @param channel_id Channel to add reactions to
	 * @param reaction Reaction to add. Emojis should be in the form emojiname:id
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_add_reaction(snowflake message_id, snowflake channel_id, const std::string &reaction, command_completion_event_t callback = {});

	/**
	 * @brief Delete own reaction from a message by id. The reaction string must be either an `emojiname:id` or a unicode character.
	 *
	 * @param message_id Message to delete reactions from
	 * @param channel_id Channel to delete reactions from
	 * @param reaction Reaction to delete. The reaction should be in the form emojiname:id
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete_own_reaction(snowflake message_id, snowflake channel_id, const std::string &reaction, command_completion_event_t callback = {});

	/**
	 * @brief Delete a user's reaction from a message by id. The reaction string must be either an `emojiname:id` or a unicode character
	 *
	 * @param message_id Message to delete reactions from
	 * @param channel_id Channel to delete reactions from
	 * @param user_id User ID who's reaction you want to remove
	 * @param reaction Reaction to remove. Reactions should be in the form emojiname:id
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete_reaction(snowflake message_id, snowflake channel_id, snowflake user_id, const std::string &reaction, command_completion_event_t callback = {});

	/**
	 * @brief Get reactions on a message for a particular emoji by id. The reaction string must be either an `emojiname:id` or a unicode character
	 *
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
	 * @param message_id Message to delete reactions from
	 * @param channel_id Channel to delete reactions from
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete_all_reactions(snowflake message_id, snowflake channel_id, command_completion_event_t callback = {});

	/**
	 * @brief Delete all reactions on a message using a particular emoji by id. The reaction string must be either an `emojiname:id` or a unicode character
	 *
	 * @param message_id Message to delete reactions from
	 * @param channel_id Channel to delete reactions from
	 * @param reaction Reaction to delete, in the form emojiname:id or a unicode character
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete_reaction_emoji(snowflake message_id, snowflake channel_id, const std::string &reaction, command_completion_event_t callback = {});

	/**
	 * @brief Delete a message from a channel. The callback function is called when the message has been edited
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param message_id Message ID to delete
	 * @param channel_id Channel to delete from
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete(snowflake message_id, snowflake channel_id, command_completion_event_t callback = {});

	/**
	 * @brief Bulk delete messages from a channel. The callback function is called when the message has been edited
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param message_ids List of message IDs to delete (maximum of 100 message IDs)
	 * @param channel_id Channel to delete from
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_delete_bulk(const std::vector<snowflake> &message_ids, snowflake channel_id, command_completion_event_t callback = {});

	/**
	 * @brief Get a channel
	 *
	 * @param c Channel ID to retrieve
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::channel object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_get(snowflake c, command_completion_event_t callback);

	/**
	 * @brief Get all channels for a guild
	 *
	 * @param guild_id Guild ID to retrieve channels for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::channel_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channels_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Create a channel
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param c Channel to create
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::channel object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_create(const class channel &c, command_completion_event_t callback = {});

	/**
	 * @brief Edit a channel
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param c Channel to edit/update
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::channel object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_edit(const class channel &c, command_completion_event_t callback = {});

	/**
	 * @brief Edit a channel's position
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param c Channel to change the position for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::channel object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_edit_position(const class channel &c, command_completion_event_t callback = {});

	/**
	 * @brief Edit a channel's permissions
	 *
	 * @param c Channel to set permissions for
	 * @param overwrite_id Overwrite to change (a user or channel ID)
	 * @param allow allow permissions
	 * @param deny deny permissions
	 * @param member true if the overwrite_id is a user id, false if it is a channel id
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_edit_permissions(const class channel &c, snowflake overwrite_id, uint32_t allow, uint32_t deny, bool member, command_completion_event_t callback = {});

	/**
	 * @brief Delete a channel
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param channel_id Channel id to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_delete(snowflake channel_id, command_completion_event_t callback = {});

	/**
	 * @brief Get details about an invite
	 *
	 * @param invite Invite code to get information on
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::invite object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void invite_get(const std::string &invite, command_completion_event_t callback);

	/**
	 * @brief Delete an invite
	 *
	 * @param invite Invite code to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void invite_delete(const std::string &invite, command_completion_event_t callback = {});

	/**
	 * @brief Get invites for a channel
	 *
	 * @param c Channel to get invites for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::invite_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_invites_get(const class channel &c, command_completion_event_t callback);

	/**
	 * @brief Create invite for a channel
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param c Channel to create an invite on
	 * @param i Invite to create
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::invite object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_invite_create(const class channel &c, const class invite &i, command_completion_event_t callback = {});

	/**
	 * @brief Get a channel's pins
	 *
	 * @param channel_id Channel ID to get pins for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_pins_get(snowflake channel_id, command_completion_event_t callback);

	/**
	 * @brief Adds a recipient to a Group DM using their access token
	 *
	 * @param channel_id Channel id to add group DM recipients to
	 * @param user_id User ID to add
	 * @param access_token Access token from OAuth2
	 * @param nick Nickname of user to apply to the chat
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void gdm_add(snowflake channel_id, snowflake user_id, const std::string &access_token, const std::string &nick, command_completion_event_t callback = {});

	/**
	 * @brief Removes a recipient from a Group DM
	 *
	 * @param channel_id Channel ID of group DM
	 * @param user_id User ID to remove from group DM
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void gdm_remove(snowflake channel_id, snowflake user_id, command_completion_event_t callback = {});

	/**
	 * @brief Remove a permission from a channel
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param c Channel to remove permission from
	 * @param overwrite_id Overwrite to remove, user or channel ID
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_delete_permission(const class channel &c, snowflake overwrite_id, command_completion_event_t callback = {});

	/**
	 * @brief Follow a news channel
	 *
	 * @param c Channel id to follow
	 * @param target_channel_id Channel to subscribe the channel to
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_follow_news(const class channel &c, snowflake target_channel_id, command_completion_event_t callback = {});

	/**
	 * @brief Trigger channel typing indicator
	 *
	 * @param c Channel to set as typing on
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_typing(const class channel &c, command_completion_event_t callback = {});

	/**
	 * @brief Trigger channel typing indicator
	 *
	 * @param cid Channel ID to set as typing on
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void channel_typing(snowflake cid, command_completion_event_t callback = {});

	/**
	 * @brief Pin a message
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param channel_id Channel id to pin message on
	 * @param message_id Message id to pin message on
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_pin(snowflake channel_id, snowflake message_id, command_completion_event_t callback = {});

	/**
	 * @brief Unpin a message
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param channel_id Channel id to unpin message on
	 * @param message_id Message id to unpin message on
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void message_unpin(snowflake channel_id, snowflake message_id, command_completion_event_t callback = {});

	/**
	 * @brief Get a guild
	 *
	 * @param g Guild ID to retrieve
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get(snowflake g, command_completion_event_t callback);

	/**
	 * @brief Get a guild preview. Returns a guild object but only a subset of the fields will be populated.
	 *
	 * @param g Guild ID to retrieve
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_preview(snowflake g, command_completion_event_t callback);

	/**
	 * @brief Get a guild member
	 *
	 * @param guild_id Guild ID to get member for
	 * @param user_id User ID of member to get
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_member object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_member(snowflake guild_id, snowflake user_id, command_completion_event_t callback);

	/**
	 * @brief Search for guild members based on whether their username or nickname starts with the given string.
	 *
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
	 * @param gm Guild member to add
	 * @param access_token Access token from Oauth2 scope
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_member object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_add_member(const guild_member& gm, const std::string &access_token, command_completion_event_t callback = {});

	/**
	 * @brief Edit the properties of an existing guild member
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param gm Guild member to edit
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_member object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_edit_member(const guild_member& gm, command_completion_event_t callback = {});

	/**
	 * @brief Moves the guild member to a other voice channel, if member is connected to one
	 * @param channel_id Id of the channel to which the user is used
	 * @param guild_id Guild id to which the user is connected
	 * @param user_id User id, who should be moved
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_member object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_member_move(const snowflake channel_id, const snowflake guild_id, const snowflake user_id, command_completion_event_t callback = {});

	/**
	 * @brief Change current user nickname
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param guild_id Guild ID to change nickanem on
	 * @param nickname New nickname, or empty string to clear nickname
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_set_nickname(snowflake guild_id, const std::string &nickname, command_completion_event_t callback = {});

	/**
	 * @brief Add role to guild member
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param guild_id Guild ID to add a role to
	 * @param user_id User ID to add role to
	 * @param role_id Role ID to add to the user
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_member_add_role(snowflake guild_id, snowflake user_id, snowflake role_id, command_completion_event_t callback = {});

	/**
	 * @brief Remove role from guild member
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param guild_id Guild ID to remove role from user on
	 * @param user_id User ID to remove role from
	 * @param role_id Role to remove
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_member_delete_role(snowflake guild_id, snowflake user_id, snowflake role_id, command_completion_event_t callback = {});

	/**
	 * @brief Remove (kick) a guild member
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param guild_id Guild ID to kick member from
	 * @param user_id User ID to kick
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_member_delete(snowflake guild_id, snowflake user_id, command_completion_event_t callback = {});

	/**
	 * @brief Add guild ban
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param guild_id Guild ID to add ban to
	 * @param user_id User ID to ban
	 * @param delete_message_days How many days of ther user's messages to also delete
	 * @param reason Reason for ban
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::ban object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_ban_add(snowflake guild_id, snowflake user_id, uint32_t delete_message_days, const std::string &reason, command_completion_event_t callback = {});

	/**
	 * @brief Delete guild ban
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param guild_id Guild to delete ban from
	 * @param user_id User ID to delete ban for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_ban_delete(snowflake guild_id, snowflake user_id, command_completion_event_t callback = {});

	/**
	 * @brief Get guild ban list
	 *
	 * @param guild_id Guild ID to get bans for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::ban_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_bans(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get single guild ban
	 *
	 * @param guild_id Guild ID to get ban for
	 * @param user_id User ID of ban to retrieve
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::ban object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_ban(snowflake guild_id, snowflake user_id, command_completion_event_t callback);

	/**
	 * @brief Get a template
	 *
	 * @param code Template code
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void template_get(const std::string &code, command_completion_event_t callback);

	/**
	 * @brief Create a new guild based on a template.
	 *
	 * @param code Template code to create guild from
	 * @param name Guild name to create
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_create_from_template(const std::string &code, const std::string &name, command_completion_event_t callback = {});

	/**
	 * @brief Get guild templates
	 *
	 * @param guild_id Guild ID to get templates for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_templates_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Creates a template for the guild
	 *
	 * @param guild_id Guild to create template from
	 * @param name Template name to create
	 * @param description Description of template to create
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_template_create(snowflake guild_id, const std::string &name, const std::string &description, command_completion_event_t callback);

	/**
	 * @brief Syncs the template to the guild's current state.
	 *
	 * @param guild_id Guild to synchronise template for
	 * @param code Code of template to synchronise
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_template_sync(snowflake guild_id, const std::string &code, command_completion_event_t callback = {});

	/**
	 * @brief Modifies the template's metadata.
	 *
	 * @param guild_id Guild ID of template to modify
	 * @param code Template code to modify
	 * @param name New name of template
	 * @param description New description of template
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_template_modify(snowflake guild_id, const std::string &code, const std::string &name, const std::string &description, command_completion_event_t callback = {});

	/**
	 * @brief Deletes the template
	 *
	 * @param guild_id Guild ID of template to delete
	 * @param code Template code to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_template_delete(snowflake guild_id, const std::string &code, command_completion_event_t callback = {});

	/**
	 * @brief Create a guild
	 *
	 * @param g Guild to create
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_create(const class guild &g, command_completion_event_t callback = {});

	/**
	 * @brief Edit a guild
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param g Guild to edit
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_edit(const class guild &g, command_completion_event_t callback = {});

	/**
	 * @brief Delete a guild
	 *
	 * @param guild_id Guild ID to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_delete(snowflake guild_id, command_completion_event_t callback = {});

	/**
	 * @brief Get all emojis for a guild
	 *
	 * @param guild_id Guild ID to get emojis for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::emoji_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_emojis_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get a single emoji
	 *
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
	 * @param guild_id Guild ID to create emoji om
	 * @param newemoji Emoji to create
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::emoji object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_emoji_create(snowflake guild_id, const class emoji& newemoji, command_completion_event_t callback = {});

	/**
	 * @brief Edit a single emoji.
	 * You must ensure that the emoji passed contained image data using the emoji::load_image() method.
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param guild_id Guild ID to edit emoji on
	 * @param newemoji Emoji to edit
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::emoji object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_emoji_edit(snowflake guild_id, const class emoji& newemoji, command_completion_event_t callback = {});

	/**
	 * @brief Delete a guild emoji
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param guild_id Guild ID to delete emoji on
	 * @param emoji_id Emoji ID to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_emoji_delete(snowflake guild_id, snowflake emoji_id, command_completion_event_t callback = {});

	/**
	 * @brief Get prune counts
	 *
	 * @param guild_id Guild ID to count for pruning
	 * @param pruneinfo Pruning info
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::prune object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_prune_counts(snowflake guild_id, const struct prune& pruneinfo, command_completion_event_t callback);

	/**
	 * @brief Begin guild prune
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param guild_id Guild ID to prune
	 * @param pruneinfo Pruning info
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::prune object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_begin_prune(snowflake guild_id, const struct prune& pruneinfo, command_completion_event_t callback = {});

	/**
	 * @brief Get guild voice regions.
	 * Voice regions per guild are somewhat deprecated in preference of per-channel voice regions.
	 *
	 * @param guild_id Guild ID to get voice regions for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::voiceregion_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_voice_regions(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get guild invites
	 *
	 * @param guild_id Guild ID to get invites for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::invite_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_invites(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get guild itegrations
	 *
	 * @param guild_id Guild ID to get integrations for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::integration_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_integrations(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Modify guild integration
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param guild_id Guild ID to modify integration for
	 * @param i Integration to modify
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::integration object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_modify_integration(snowflake guild_id, const class integration &i, command_completion_event_t callback = {});

	/**
	 * @brief Delete guild integration
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param guild_id Guild ID to delete integration for
	 * @param integration_id Integration ID to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_delete_integration(snowflake guild_id, snowflake integration_id, command_completion_event_t callback = {});

	/**
	 * @brief Sync guild integration
	 *
	 * @param guild_id Guild ID to sync integration on
	 * @param integration_id Integration ID to synchronise
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_sync_integration(snowflake guild_id, snowflake integration_id, command_completion_event_t callback = {});

	/**
	 * @brief Get guild widget
	 *
	 * @param guild_id Guild ID to get widget for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_widget object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_widget(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Edit guild widget
	 *
	 * @param guild_id Guild ID to edit widget for
	 * @param gw New guild widget information
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_widget object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_edit_widget(snowflake guild_id, const class guild_widget &gw, command_completion_event_t callback = {});

	/**
	 * @brief Get guild vanity url, if enabled
	 *
	 * @param guild_id Guild to get vanity URL for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::invite object in confirmation_callback_t::value filled to match the vanity url. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_get_vanity(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Create a webhook
	 *
	 * @param w Webhook to create
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::webhook object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void create_webhook(const class webhook &w, command_completion_event_t callback = {});

	/**
	 * @brief Get guild webhooks
	 *
	 * @param guild_id Guild ID to get webhooks for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::webhook_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void get_guild_webhooks(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get channel webhooks
	 *
	 * @param channel_id Channel ID to get webhooks for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::webhook_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void get_channel_webhooks(snowflake channel_id, command_completion_event_t callback);

	/**
	 * @brief Get webhook
	 *
	 * @param webhook_id Webhook ID to get
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::webhook object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void get_webhook(snowflake webhook_id, command_completion_event_t callback);

	/**
	 * @brief Get webhook using token
	 *
	 * @param webhook_id Webhook ID to retrieve
	 * @param token Token of webhook
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::webhook object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void get_webhook_with_token(snowflake webhook_id, const std::string &token, command_completion_event_t callback);

	/**
	 * @brief Edit webhook
	 *
	 * @param wh Webhook to edit
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::webhook object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void edit_webhook(const class webhook& wh, command_completion_event_t callback = {});

	/**
	 * @brief Edit webhook with token (token is encapsulated in the webhook object)
	 *
	 * @param wh Wehook to edit (should include token)
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::webhook object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void edit_webhook_with_token(const class webhook& wh, command_completion_event_t callback = {});

	/**
	 * @brief Delete a webhook
	 *
	 * @param webhook_id Webhook ID to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void delete_webhook(snowflake webhook_id, command_completion_event_t callback = {});

	/**
	 * @brief Delete webhook with token
	 *
	 * @param webhook_id Webhook ID to delete
	 * @param token Token of webhook to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void delete_webhook_with_token(snowflake webhook_id, const std::string &token, command_completion_event_t callback = {});

	/**
	 * @brief Execute webhook
	 *
	 * @param wh Webhook to execute
	 * @param m Message to send
	 * @param wait waits for server confirmation of message send before response, and returns the created message body
	 * @param thread_id Send a message to the specified thread within a webhook's channel. The thread will automatically be unarchived
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void execute_webhook(const class webhook &wh, const struct message &m, bool wait = false, snowflake thread_id = 0, command_completion_event_t callback = {});

	/**
	 * @brief Get webhook message
	 *
	 * @param wh Webhook to get the original message for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void get_webhook_message(const class webhook &wh, command_completion_event_t callback = {});

	/**
	 * @brief Edit webhook message
	 *
	 * @param wh Webhook to edit message for
	 * @param m New message
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void edit_webhook_message(const class webhook &wh, const struct message &m, command_completion_event_t callback = {});

	/**
	 * @brief Delete webhook message
	 *
	 * @param wh Webhook to delete message for
	 * @param message_id Message ID to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void delete_webhook_message(const class webhook &wh, snowflake message_id, command_completion_event_t callback = {});

	/**
	 * @brief Get a role for a guild
	 *
	 * @param guild_id Guild ID to get role for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::role_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void roles_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Create a role on a guild
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param r Role to create (guild ID is encapsulated in the role object)
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::role object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void role_create(const class role &r, command_completion_event_t callback = {});

	/**
	 * @brief Edit a role on a guild
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param r Role to edit
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::role object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void role_edit(const class role &r, command_completion_event_t callback = {});

	/**
	 * @brief Edit a role's position in a guild
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param r Role to change position of
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::role object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void role_edit_position(const class role &r, command_completion_event_t callback = {});

	/**
	 * @brief Delete a role
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param guild_id Guild ID to delete the role on
	 * @param role_id Role ID to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void role_delete(snowflake guild_id, snowflake role_id, command_completion_event_t callback = {});

	/**
	 * @brief Get a user by id
	 *
	 * @param user_id User ID to retrieve
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::user_identified object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 * @note The user_identified object is a subclass of dpp::user which contains further details if you have the oauth2 identify or email scopes.
	 * If you do not have these scopes, these fields are empty. You can safely convert a user_identified to user with `dynamic_cast`.
	 */
	void user_get(snowflake user_id, command_completion_event_t callback);

	/**
	 * @brief Get current (bot) user
	 *
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::user_identified object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 * @note The user_identified object is a subclass of dpp::user which contains further details if you have the oauth2 identify or email scopes.
	 * If you do not have these scopes, these fields are empty. You can safely convert a user_identified to user with `dynamic_cast`.
	 */
	void current_user_get(command_completion_event_t callback);

	/**
	 * @brief Get current (bot) application
	 *
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::application object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void current_application_get(command_completion_event_t callback);

	/**
	 * @brief Get current user's connections (linked accounts, e.g. steam, xbox).
	 * This call requires the oauth2 `connections` scope and cannot be executed
	 * against a bot token.
	 *
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::connection_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void current_user_connections_get(command_completion_event_t callback);

	/**
	 * @brief Get current (bot) user guilds
	 *
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::guild_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void current_user_get_guilds(command_completion_event_t callback);

	/**
	 * @brief Edit current (bot) user
	 *
	 * @param nickname Nickname to set
	 * @param image_blob Avatar data to upload (NOTE: Very heavily rate limited!)
	 * @param type Type of image for avatar
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::user object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
 	 * @throw dpp::exception Image data is larger than the maximum size of 256 kilobytes
	 */
	void current_user_edit(const std::string &nickname, const std::string& image_blob = "", const image_type type = i_png, command_completion_event_t callback = {});

	/**
	 * @brief Get current user DM channels
	 *
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::channel_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void current_user_get_dms(command_completion_event_t callback);

	/**
	 * @brief Create a dm channel
	 *
	 * @param user_id User ID to create DM channel with
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::channel object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void create_dm_channel(snowflake user_id, command_completion_event_t callback = {});

	/**
	 * @brief Leave a guild
	 *
	 * @param guild_id Guild ID to leave
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void current_user_leave_guild(snowflake guild_id, command_completion_event_t callback = {});

	/**
	 * @brief Create a thread
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param thread_name Name of the thread
	 * @param channel_id Channel in which thread to create
	 * @param auto_archive_duration Duration after which thread auto-archives. Can be set to - 60, 1440 (for boosted guilds can also be: 4320, 10080)
	 * @param thread_type Type of thread - GUILD_PUBLIC_THREAD, GUILD_NEWS_THREAD, GUILD_PRIVATE_THREAD
	 * @param invitable whether non-moderators can add other non-moderators to a thread; only available when creating a private thread
	 * @param rate_limit_per_user amount of seconds a user has to wait before sending another message (0-21600); bots, as well as users with the permission manage_messages, manage_thread, or manage_channel, are unaffected
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::thread object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void thread_create(const std::string& thread_name, snowflake channel_id, uint16_t auto_archive_duration, channel_type thread_type, bool invitable, uint16_t rate_limit_per_user, command_completion_event_t callback = {});

	/**
	 * @brief Create a thread with a message (Discord: ID of a thread is same as message ID)
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param thread_name Name of the thread
	 * @param channel_id Channel in which thread to create
	 * @param message_id message to start thread with
	 * @param auto_archive_duration Duration after which thread auto-archives. Can be set to - 60, 1440 (for boosted guilds can also be: 4320, 10080)
	 * @param rate_limit_per_user amount of seconds a user has to wait before sending another message (0-21600); bots, as well as users with the permission manage_messages, manage_thread, or manage_channel, are unaffected
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::thread object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void thread_create_with_message(const std::string& thread_name, snowflake channel_id, snowflake message_id, uint16_t auto_archive_duration, uint16_t rate_limit_per_user, command_completion_event_t callback = {});

	/**
	 * @brief Join a thread
	 *
	 * @param thread_id Thread ID to join
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void current_user_join_thread(snowflake thread_id, command_completion_event_t callback = {});

	/**
	 * @brief Leave a thread
	 *
	 * @param thread_id Thread ID to leave
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void current_user_leave_thread(snowflake thread_id, command_completion_event_t callback = {});

	/**
	 * @brief Add a member to a thread
	 *
	 * @param thread_id Thread ID to add to
	 * @param user_id Member ID to add
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void thread_member_add(snowflake thread_id, snowflake user_id, command_completion_event_t callback = {});

	/**
	 * @brief Remove a member from a thread
	 *
	 * @param thread_id Thread ID to remove from
	 * @param user_id Member ID to remove
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void thread_member_remove(snowflake thread_id, snowflake user_id, command_completion_event_t callback = {});

	/**
	 * @brief Get a thread member
	 *
	 * @param thread_id Thread to get member for
	 * @param user_id ID of the user to get
	 * @param callback Function to call when the API call completes
	 * On success the callback will contain a dpp::thread_member object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void thread_member_get(const snowflake thread_id, const snowflake user_id, command_completion_event_t callback);

	/**
	 * @brief Get members of a thread
	 *
	 * @param thread_id Thread to get members for
	 * @param callback Function to call when the API call completes
	 * On success the callback will contain a dpp::thread_member_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void thread_members_get(snowflake thread_id, command_completion_event_t callback);

	/**
	 * @brief Get active threads in a channel (Sorted by ID in descending order)
	 *
	 * @param channel_id Channel to get active threads for
	 * @param callback Function to call when the API call completes
	 * On success the callback will contain a dpp::thread_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void threads_get_active(snowflake channel_id, command_completion_event_t callback);

	/**
	 * @brief Get public archived threads in a channel (Sorted by archive_timestamp in descending order)
	 *
	 * @param channel_id Channel to get public archived threads for
	 * @param before_timestamp Get threads before this timestamp
	 * @param limit Number of threads to get
	 * @param callback Function to call when the API call completes
	 * On success the callback will contain a dpp::thread_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void threads_get_public_archived(snowflake channel_id, time_t before_timestamp, uint16_t limit, command_completion_event_t callback);

	/**
	 * @brief Get private archived threads in a channel (Sorted by archive_timestamp in descending order)
	 *
	 * @param channel_id Channel to get public archived threads for
	 * @param before_timestamp Get threads before this timestamp
	 * @param limit Number of threads to get
	 * @param callback Function to call when the API call completes
	 * On success the callback will contain a dpp::thread_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void threads_get_private_archived(snowflake channel_id,  time_t before_timestamp, uint16_t limit, command_completion_event_t callback);

	/**
	 * @brief Get private archived threads in a channel which current user has joined (Sorted by ID in descending order)

	 *
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
	 *
	 * @param s Sticker to create. Must have its guild ID set.
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::sticker object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_sticker_create(sticker &s, command_completion_event_t callback = {});

	/**
	 * @brief Modify a sticker in a guild
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param s Sticker to modify. Must have its guild ID and sticker ID set.
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::sticker object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_sticker_modify(sticker &s, command_completion_event_t callback = {});

	/**
	 * @brief Delete a sticker from a guild
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 *
	 * @param sticker_id sticker ID to delete
	 * @param guild_id guild ID to delete from
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::sticker object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_sticker_delete(snowflake sticker_id, snowflake guild_id, command_completion_event_t callback = {});

	/**
	 * @brief Get a nitro sticker
	 *
	 * @param id Id of sticker to get.
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::sticker object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void nitro_sticker_get(snowflake id, command_completion_event_t callback);

	/**
	 * @brief Get a guild sticker
	 *
	 * @param id Id of sticker to get.
	 * @param guild_id Guild ID of the guild where the sticker is
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::sticker object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_sticker_get(snowflake id, snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get all guild stickers
	 *
	 * @param guild_id Guild ID of the guild where the sticker is
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::sticker object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_stickers_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get sticker packs
	 *
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::sticker object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void sticker_packs_get(command_completion_event_t callback);

	/**
	 * @brief Create a stage instance on a stage channel.
	 *
	 * @param instance Stage instance to create
	 * @param callback User function to execute when the api call completes
	 * On success the callback will contain a dpp::stage_instance object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 */
	void stage_instance_create(const stage_instance& instance, command_completion_event_t callback = {});

	/**
	 * @brief Get the stage instance associated with the channel id, if it exists.
	 *
	 * @param channel_id ID of the associated channel
	 * @param callback User function to execute when the api call completes
	 * On success the callback will contain a dpp::stage_instance object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void stage_instance_get(const snowflake channel_id, command_completion_event_t callback);

	/**
	 * @brief Edit a stage instance.
	 *
	 * @param instance Stage instance to edit
	 * @param callback User function to execute when the api call completes
	 * On success the callback will contain a dpp::stage_instance object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 */
	void stage_instance_edit(const stage_instance& instance, command_completion_event_t callback = {});

	/**
	 * @brief Delete a stage instance.
	 *
	 * @param channel_id ID of the associated channel
	 * @param callback User function to execute when the api call completes
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 * @note This method supports audit log reasons set by the cluster::set_audit_reason() method.
	 */
	void stage_instance_delete(const snowflake channel_id, command_completion_event_t callback = {});

	/**
	 * @brief Get all voice regions
	 *
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::voiceregion_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void get_voice_regions(command_completion_event_t callback);

	/**
	 * @brief Get the gateway information for the bot using the token
	 *
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::gateway object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void get_gateway_bot(command_completion_event_t callback);

	/**
	 * @brief Get all scheduled events for a guild
	 *
	 * @param guild_id Guild to get events for
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::scheduled_event_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_events_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get users RSVP'd to an event
	 *
	 * @param guild_id Guild to get user list for
	 * @param event_id Guild to get user list for
	 * @param limit Maximum number of results to return
	 * @param before Return user IDs that fall before this ID, if provided
	 * @param after Return user IDs that fall after this ID, if provided
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::user_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_event_users_get(snowflake guild_id, snowflake event_id, command_completion_event_t callback, uint8_t limit = 100, snowflake before = 0, snowflake after = 0);

	/**
	 * @brief Create a scheduled event on a guild
	 *
	 * @param event Event to create (guild ID must be populated)
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::scheduled_event_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_event_create(const scheduled_event& event, command_completion_event_t callback = {});

	/**
	 * @brief Delete a scheduled event from a guild
	 *
	 * @param event_id Event ID to delete
	 * @param guild_id Guild ID of event to delete
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::confirmation object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_event_delete(snowflake event_id, snowflake guild_id, command_completion_event_t callback = {});

	/**
	 * @brief Edit/modify a scheduled event on a guild
	 *
	 * @param event Event to create (event ID and guild ID must be populated)
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::scheduled_event_map object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_event_edit(const scheduled_event& event, command_completion_event_t callback = {});

	/**
	 * @brief Get a scheduled event for a guild
	 *
	 * @param guild_id Guild to get event for
	 * @param event_id Event ID to get
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::scheduled_event object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void guild_event_get(snowflake guild_id, snowflake event_id, command_completion_event_t callback);


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
	std::map<event_handle, attached_event>& ev;

	/// Timer handle
	timer th;

	/// Event handle
	event_handle listener_handler;
    
public:
	/**
	 * @brief Construct a new timed listener object
	 * 
	 * @param cl Owning cluster
	 * @param dur Duration of timed event in seconds
	 * @param event Event to hook, e.g. cluster->dispatch.message_create
	 * @param on_end An optional void() lambda to trigger when the timed_listener times out.
	 * Calling the destructor before the timeout is reached does not call this lambda.
	 * @param listener Lambda to receive events. Type must match up properly with that passed into the 'event' parameter.
	 */
	timed_listener(cluster* cl, uint64_t _duration, std::map<event_handle, attached_event>& event, listening_function listener, timer_callback_t on_end = {})
	: owner(cl), duration(_duration), ev(event)
	{
		/* Attach event */
		listener_handler = owner->get_next_handle();
		event.emplace(listener_handler, listener);
		/* Create timer */
		th = cl->start_timer([this]() {
			/* Timer has finished, detach it from event.
			 * Only allowed to tick once.
			 */
			owner->stop_timer(th);
			dpp::detach(ev, listener_handler);
		}, duration, on_end);
	}

	/**
	 * @brief Destroy the timed listener object
	 */
	~timed_listener() {
		/* Stop timer and detach event, but do not call on_end */
		owner->stop_timer(th);
		dpp::detach(ev, listener_handler);
	}
};

};
