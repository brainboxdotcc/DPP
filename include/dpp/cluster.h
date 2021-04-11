#pragma once

#include <string>
#include <map>
#include <variant>
#include <dpp/discord.h>
#include <dpp/dispatcher.h>
#include <dpp/json_fwd.hpp>
#include <dpp/discordclient.h>
#include <dpp/queues.h>

using  json = nlohmann::json;

namespace dpp {

/**
 * @brief Represents the various information from the 'get gateway bot' api call
 */
struct gateway {
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
struct confirmation {
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
		user_map,
		guild_member,
		guild_member_map,
		channel,
		channel_map,
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
		gateway
	> confirmable_t;

/**
 * @brief The results of a REST call wrapped in a convenient struct
 */
struct confirmation_callback_t {
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
class cluster {

	/** queue system for commands sent to Discord, and any replies */
	request_queue* rest;

	/** True if to use compression on shards */
	bool compressed;

	/**
	 * @brief Lock to prevent concurrent access to dm_channels
	 */
	std::mutex dm_list_lock;

	/**
	 * @brief Active DM channels for the bot
	 */
	std::unordered_map<snowflake, snowflake> dm_channels;

	/**
	 * @brief Accepts result from /gateway/bot REST API call and populates numshards with it
	 * 
	 * @param shardinfo Received HTTP data from API call
	 */
	void auto_shard(const confirmation_callback_t &shardinfo);
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

	/** Routes events from Discord back to user program code via std::functions */
	dpp::dispatcher dispatch;

	/**
	 * @brief Active shards on this cluster. Shard IDs may have gaps between if there 
	 * are multiple clusters.
	 */
	std::map<uint32_t, class DiscordClient*> shards;

	/** 
	 * @brief The details of the bot user. This is assumed to be identical across all shards
	 * in the cluster. Each connecting shard updates this information.
	 */
	dpp::user me;

	/** 
	 * @brief Constructor for creating a cluster. All but the token are optional.
	 * @param token The bot token to use for all HTTP commands and websocket connections
	 * @param intents A bitmask of dpd::intents values for all shards on this cluster. This is required to be sent for all bots with over 100 servers.
	 * @param shards The total number of shards on this bot. If there are multiple clusters, then (shards / clusters) actual shards will run on this cluster.
	 * If you omit this value, the library will attempt to query the Discord API for the correct number of shards to start.
	 * @param cluster_id The ID of this cluster, should be between 0 and MAXCLUSTERS-1
	 * @param maxclusters The total number of clusters that are active, which may be on seperate processes or even separate machines.
	 * @param compressed Wether or not to use compression for shards on this cluster. Saves a ton of bandwidth at the cost of some CPU
	 */
	cluster(const std::string &token, uint32_t intents = i_default_intents, uint32_t shards = 0, uint32_t cluster_id = 0, uint32_t maxclusters = 1, bool compressed = true);

	/** Destructor */
	~cluster();

	/** 
	 * @brief Log a message to whatever log the user is using.
	 * The logged message is passed up the chain to the on_log event in user code which can then do whatever
	 * it wants to do with it.
	 * @param severity The log level from dpp::loglevel
	 * @param msg The log message to output
	 */
	void log(dpp::loglevel severity, const std::string &msg);

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

	/* Functions for attaching to event handlers */

	/**
	 * @brief on voice state update event
	 * 
	 * @param _voice_state_update User function to attach to event
	 */
	void on_voice_state_update (std::function<void(const voice_state_update_t& _event)> _voice_state_update);

	/**
	 * @brief 
	 * 
	 * @param _log  User function to attach to event
	 */
	void on_log (std::function<void(const log_t& _event)> _log);

	/**
	 * @brief 
	 * 
	 * @param _interaction_create  User function to attach to event
	 */
	void on_interaction_create (std::function<void(const interaction_create_t& _event)> _interaction_create);

	/**
	 * @brief 
	 * 
	 * @param _guild_delete  User function to attach to event
	 */
	void on_guild_delete (std::function<void(const guild_delete_t& _event)> _guild_delete);

	/**
	 * @brief 
	 * 
	 * @param _channel_delete  User function to attach to event
	 */
	void on_channel_delete (std::function<void(const channel_delete_t& _event)> _channel_delete);

	/**
	 * @brief 
	 * 
	 * @param _channel_update  User function to attach to event
	 */
	void on_channel_update (std::function<void(const channel_update_t& _event)> _channel_update);

	/**
	 * @brief 
	 * 
	 * @param _ready  User function to attach to event
	 */
	void on_ready (std::function<void(const ready_t& _event)> _ready);

	/**
	 * @brief 
	 * 
	 * @param _message_delete  User function to attach to event
	 */
	void on_message_delete (std::function<void(const message_delete_t& _event)> _message_delete);

	/**
	 * @brief 
	 * 
	 * @param _application_command_delete  User function to attach to event
	 */
	void on_application_command_delete (std::function<void(const application_command_delete_t& _event)> _application_command_delete);

	/**
	 * @brief 
	 * 
	 * @param _guild_member_remove  User function to attach to event
	 */
	void on_guild_member_remove (std::function<void(const guild_member_remove_t& _event)> _guild_member_remove);

	/**
	 * @brief 
	 * 
	 * @param _application_command_create  User function to attach to event
	 */
	void on_application_command_create (std::function<void(const application_command_create_t& _event)> _application_command_create);

	/**
	 * @brief 
	 * 
	 * @param _resumed  User function to attach to event
	 */
	void on_resumed (std::function<void(const resumed_t& _event)> _resumed);

	/**
	 * @brief 
	 * 
	 * @param _guild_role_create  User function to attach to event
	 */
	void on_guild_role_create (std::function<void(const guild_role_create_t& _event)> _guild_role_create);

	/**
	 * @brief 
	 * 
	 * @param _typing_start  User function to attach to event
	 */
	void on_typing_start (std::function<void(const typing_start_t& _event)> _typing_start);

	/**
	 * @brief 
	 * 
	 * @param _message_reaction_add  User function to attach to event
	 */
	void on_message_reaction_add (std::function<void(const message_reaction_add_t& _event)> _message_reaction_add);

	/**
	 * @brief 
	 * 
	 * @param _guild_members_chunk  User function to attach to event
	 */
	void on_guild_members_chunk (std::function<void(const guild_members_chunk_t& _event)> _guild_members_chunk);

	/**
	 * @brief 
	 * 
	 * @param _message_reaction_remove  User function to attach to event
	 */
	void on_message_reaction_remove (std::function<void(const message_reaction_remove_t& _event)> _message_reaction_remove);

	/**
	 * @brief 
	 * 
	 * @param _guild_create  User function to attach to event
	 */
	void on_guild_create (std::function<void(const guild_create_t& _event)> _guild_create);

	/**
	 * @brief 
	 * 
	 * @param _channel_create  User function to attach to event
	 */
	void on_channel_create (std::function<void(const channel_create_t& _event)> _channel_create);

	/**
	 * @brief 
	 * 
	 * @param _message_reaction_remove_emoji  User function to attach to event
	 */
	void on_message_reaction_remove_emoji (std::function<void(const message_reaction_remove_emoji_t& _event)> _message_reaction_remove_emoji);

	/**
	 * @brief 
	 * 
	 * @param _message_delete_bulk  User function to attach to event
	 */
	void on_message_delete_bulk (std::function<void(const message_delete_bulk_t& _event)> _message_delete_bulk);

	/**
	 * @brief 
	 * 
	 * @param _guild_role_update  User function to attach to event
	 */
	void on_guild_role_update (std::function<void(const guild_role_update_t& _event)> _guild_role_update);

	/**
	 * @brief 
	 * 
	 * @param _guild_role_delete  User function to attach to event
	 */
	void on_guild_role_delete (std::function<void(const guild_role_delete_t& _event)> _guild_role_delete);

	/**
	 * @brief 
	 * 
	 * @param _channel_pins_update  User function to attach to event
	 */
	void on_channel_pins_update (std::function<void(const channel_pins_update_t& _event)> _channel_pins_update);

	/**
	 * @brief 
	 * 
	 * @param _message_reaction_remove_all  User function to attach to event
	 */
	void on_message_reaction_remove_all (std::function<void(const message_reaction_remove_all_t& _event)> _message_reaction_remove_all);

	/**
	 * @brief 
	 * 
	 * @param _voice_server_update  User function to attach to event
	 */
	void on_voice_server_update (std::function<void(const voice_server_update_t& _event)> _voice_server_update);

	/**
	 * @brief 
	 * 
	 * @param _guild_emojis_update  User function to attach to event
	 */
	void on_guild_emojis_update (std::function<void(const guild_emojis_update_t& _event)> _guild_emojis_update);

	/**
	 * @brief 
	 * 
	 * @param _presence_update  User function to attach to event
	 */
	void on_presence_update (std::function<void(const presence_update_t& _event)> _presence_update);

	/**
	 * @brief 
	 * 
	 * @param _webhooks_update  User function to attach to event
	 */
	void on_webhooks_update (std::function<void(const webhooks_update_t& _event)> _webhooks_update);

	/**
	 * @brief 
	 * 
	 * @param _guild_member_add  User function to attach to event
	 */
	void on_guild_member_add (std::function<void(const guild_member_add_t& _event)> _guild_member_add);

	/**
	 * @brief 
	 * 
	 * @param _invite_delete  User function to attach to event
	 */
	void on_invite_delete (std::function<void(const invite_delete_t& _event)> _invite_delete);

	/**
	 * @brief 
	 * 
	 * @param _guild_update  User function to attach to event
	 */
	void on_guild_update (std::function<void(const guild_update_t& _event)> _guild_update);

	/**
	 * @brief 
	 * 
	 * @param _guild_integrations_update  User function to attach to event
	 */
	void on_guild_integrations_update (std::function<void(const guild_integrations_update_t& _event)> _guild_integrations_update);

	/**
	 * @brief 
	 * 
	 * @param _guild_member_update  User function to attach to event
	 */
	void on_guild_member_update (std::function<void(const guild_member_update_t& _event)> _guild_member_update);

	/**
	 * @brief 
	 * 
	 * @param _application_command_update  User function to attach to event
	 */
	void on_application_command_update (std::function<void(const application_command_update_t& _event)> _application_command_update);

	/**
	 * @brief 
	 * 
	 * @param _invite_create  User function to attach to event
	 */
	void on_invite_create (std::function<void(const invite_create_t& _event)> _invite_create);

	/**
	 * @brief 
	 * 
	 * @param _message_update  User function to attach to event
	 */
	void on_message_update (std::function<void(const message_update_t& _event)> _message_update);

	/**
	 * @brief 
	 * 
	 * @param _user_update  User function to attach to event
	 */
	void on_user_update (std::function<void(const user_update_t& _event)> _user_update);

	/**
	 * @brief 
	 * 
	 * @param _message_create  User function to attach to event
	 */
	void on_message_create (std::function<void(const message_create_t& _event)> _message_create);

	/**
	 * @brief 
	 * 
	 * @param _guild_ban_add  User function to attach to event
	 */
	void on_guild_ban_add (std::function<void(const guild_ban_add_t& _event)> _guild_ban_add);

	/**
	 * @brief 
	 * 
	 * @param _guild_ban_remove  User function to attach to event
	 */
	void on_guild_ban_remove (std::function<void(const guild_ban_remove_t& _event)> _guild_ban_remove);

	/**
	 * @brief 
	 * 
	 * @param _integration_create User function to attach to event
	 */
	void on_integration_create (std::function<void(const integration_create_t& _event)> _integration_create);

	/**
	 * @brief 
	 * 
	 * @param _integration_update User function to attach to event 
	 */
	void on_integration_update (std::function<void(const integration_update_t& _event)> _integration_update);

	/**
	 * @brief 
	 * 
	 * @param _integration_delete User function to attach to event
	 */
	void on_integration_delete (std::function<void(const integration_delete_t& _event)> _integration_delete);

	/**
	 * @brief Post a REST request. Where possible use a helper method instead like message_create
	 * 
	 * @param endpoint 
	 * @param parameters 
	 * @param method 
	 * @param postdata 
	 * @param callback Function to call when the HTTP call completes
	 * @param filename 
	 * @param filecontent 
	 */
	void post_rest(const std::string &endpoint, const std::string &parameters, http_method method, const std::string &postdata, json_encode_t callback, const std::string &filename = "", const std::string &filecontent = "");

	/**
	 * @brief Create a direct message, also create the channel for the direct message if needed
	 * 
	 * @param user_id 
	 * @param m 
	 * @param callback 
	 */
	void direct_message_create(snowflake user_id, const message &m, command_completion_event_t callback = {});

	/**
	 * @brief Get a message
	 * 
	 * @param message_id 
	 * @param channel_id 
	 * @param callback Function to call when the API call completes
	 */
	void message_get(snowflake message_id, snowflake channel_id, command_completion_event_t callback);

	/**
	 * @brief Get multiple messages 
	 * 
	 * @param channel_id 
	 * @param around 
	 * @param before 
	 * @param after 
	 * @param limit 
	 * @param callback Function to call when the API call completes
	 */
	void messages_get(snowflake channel_id, snowflake around, snowflake before, snowflake after, snowflake limit, command_completion_event_t callback);

	/**
	 * @brief Send a message to a channel. The callback function is called when the message has been sent
	 * 
	 * @param m 
	 * @param callback Function to call when the API call completes
	 */
	void message_create(const struct message &m, command_completion_event_t callback = {});

	/**
	 * @brief Crosspost a message. The callback function is called when the message has been sent
	 * 
	 * @param message_id 
	 * @param channel_id 
	 * @param callback Function to call when the API call completes
	 */
	void message_crosspost(snowflake message_id, snowflake channel_id, command_completion_event_t callback = {});

	/**
	 * @brief Edit a message on a channel. The callback function is called when the message has been edited
	 * 
	 * @param m 
	 * @param callback Function to call when the API call completes
	 */
	void message_edit(const struct message &m, command_completion_event_t callback = {});

	/**
	 * @brief Add a reaction to a message. The reaction string must be either an `emojiname:id` or a unicode character.
	 * 
	 * @param m 
	 * @param reaction 
	 * @param callback Function to call when the API call completes
	 */
	void message_add_reaction(const struct message &m, const std::string &reaction, command_completion_event_t callback = {});

	/**
	 * @brief Delete own reaction from a message. The reaction string must be either an `emojiname:id` or a unicode character.
	 * 
	 * @param m 
	 * @param reaction 
	 * @param callback Function to call when the API call completes
	 */
	void message_delete_own_reaction(const struct message &m, const std::string &reaction, command_completion_event_t callback = {});

	/**
	 * @brief Delete a user's reaction from a message. The reaction string must be either an `emojiname:id` or a unicode character
	 * 
	 * @param m 
	 * @param user_id 
	 * @param reaction 
	 * @param callback Function to call when the API call completes
	 */
	void message_delete_reaction(const struct message &m, snowflake user_id, const std::string &reaction, command_completion_event_t callback = {});

	/**
	 * @brief Get reactions on a message for a particular emoji. The reaction string must be either an `emojiname:id` or a unicode character
	 * 
	 * @param m 
	 * @param reaction 
	 * @param before 
	 * @param after 
	 * @param limit 
	 * @param callback Function to call when the API call completes
	 */
	void message_get_reactions(const struct message &m, const std::string &reaction, snowflake before, snowflake after, snowflake limit, command_completion_event_t callback);

	/**
	 * @brief Delete all reactions on a message
	 * 
	 * @param m 
	 * @param callback Function to call when the API call completes
	 */
	void message_delete_all_reactions(const struct message &m, command_completion_event_t callback = {});

	/**
	 * @brief Delete all reactions on a message using a particular emoji. The reaction string must be either an `emojiname:id` or a unicode character
	 * 
	 * @param m 
	 * @param reaction 
	 * @param callback Function to call when the API call completes
	 */
	void message_delete_reaction_emoji(const struct message &m, const std::string &reaction, command_completion_event_t callback = {});

	/**
	 * @brief Delete a message from a channel. The callback function is called when the message has been edited
	 * 
	 * @param message_id 
	 * @param channel_id 
	 * @param callback Function to call when the API call completes
	 */
	void message_delete(snowflake message_id, snowflake channel_id, command_completion_event_t callback = {});

	/**
	 * @brief Bulk delete messages from a channel. The callback function is called when the message has been edited
	 * 
	 * @param message_ids 
	 * @param channel_id 
	 * @param callback Function to call when the API call completes
	 */
	void message_delete_bulk(const std::vector<snowflake> &message_ids, snowflake channel_id, command_completion_event_t callback = {});

	/**
	 * @brief Get a channel
	 * 
	 * @param c 
	 * @param callback Function to call when the API call completes
	 */
	void channel_get(snowflake c, command_completion_event_t callback);

	/**
	 * @brief Get all channels for a guild
	 * 
	 * @param guild_id 
	 * @param callback Function to call when the API call completes
	 */
	void channels_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Create a channel
	 * 
	 * @param c 
	 * @param callback Function to call when the API call completes
	 */
	void channel_create(const class channel &c, command_completion_event_t callback = {});

	/**
	 * @brief Edit a channel
	 * 
	 * @param c 
	 * @param callback Function to call when the API call completes
	 */
	void channel_edit(const class channel &c, command_completion_event_t callback = {});

	/**
	 * @brief Edit a channel's position
	 * 
	 * @param c 
	 * @param callback Function to call when the API call completes
	 */
	void channel_edit_position(const class channel &c, command_completion_event_t callback = {});

	/**
	 * @brief Edit a channel's permissions
	 * 
	 * @param c 
	 * @param overwrite_id 
	 * @param allow 
	 * @param deny 
	 * @param member 
	 * @param callback Function to call when the API call completes
	 */
	void channel_edit_permissions(const class channel &c, snowflake overwrite_id, uint32_t allow, uint32_t deny, bool member, command_completion_event_t callback = {});

	/**
	 * @brief Delete a channel
	 * 
	 * @param channel_id 
	 * @param callback Function to call when the API call completes
	 */
	void channel_delete(snowflake channel_id, command_completion_event_t callback = {});

	/**
	 * @brief Get details about an invite
	 * 
	 * @param invite 
	 * @param callback Function to call when the API call completes
	 */
	void invite_get(const std::string &invite, command_completion_event_t callback);

	/**
	 * @brief Delete an invite
	 * 
	 * @param invite 
	 * @param callback Function to call when the API call completes
	 */
	void invite_delete(const std::string &invite, command_completion_event_t callback = {});

	/**
	 * @brief Get invites for a channel
	 * 
	 * @param c 
	 * @param callback Function to call when the API call completes
	 */
	void channel_invites_get(const class channel &c, command_completion_event_t callback);

	/**
	 * @brief Create invite for a channel
	 * 
	 * @param c 
	 * @param i 
	 * @param callback Function to call when the API call completes
	 */
	void channel_invite_create(const class channel &c, const class invite &i, command_completion_event_t callback = {});

	/**
	 * @brief Get a channel's pins
	 * 
	 * @param channel_id 
	 * @param callback Function to call when the API call completes
	 */
	void pins_get(snowflake channel_id, command_completion_event_t callback);

	/**
	 * @brief Adds a recipient to a Group DM using their access token
	 * 
	 * @param channel_id 
	 * @param user_id 
	 * @param access_token 
	 * @param nick 
	 * @param callback Function to call when the API call completes
	 */
	void gdm_add(snowflake channel_id, snowflake user_id, const std::string &access_token, const std::string &nick, command_completion_event_t callback = {});

	/**
	 * @brief Removes a recipient from a Group DM
	 * 
	 * @param channel_id 
	 * @param user_id 
	 * @param callback Function to call when the API call completes
	 */
	void gdm_remove(snowflake channel_id, snowflake user_id, command_completion_event_t callback = {});

	/**
	 * @brief Remove a permission from a channel
	 * 
	 * @param c 
	 * @param overwrite_id 
	 * @param callback Function to call when the API call completes
	 */
	void channel_delete_permission(const class channel &c, snowflake overwrite_id, command_completion_event_t callback = {});

	/**
	 * @brief Follow a news channel
	 * 
	 * @param c 
	 * @param target_channel_id 
	 * @param callback Function to call when the API call completes
	 */
	void channel_follow_news(const class channel &c, snowflake target_channel_id, command_completion_event_t callback = {});

	/**
	 * @brief Trigger channel typing indicator
	 * 
	 * @param c 
	 * @param callback Function to call when the API call completes
	 */
	void channel_typing(const class channel &c, command_completion_event_t callback = {});

	/**
	 * @brief Pin a message
	 * 
	 * @param channel_id 
	 * @param message_id 
	 * @param callback Function to call when the API call completes
	 */
	void message_pin(snowflake channel_id, snowflake message_id, command_completion_event_t callback = {});

	/**
	 * @brief Unpin a message
	 * 
	 * @param channel_id 
	 * @param message_id 
	 * @param callback Function to call when the API call completes
	 */
	void message_unpin(snowflake channel_id, snowflake message_id, command_completion_event_t callback = {});

	/**
	 * @brief Get a guild
	 * 
	 * @param g 
	 * @param callback Function to call when the API call completes
	 */
	void guild_get(snowflake g, command_completion_event_t callback);

	/**
	 * @brief Get a guild preview. Returns a guild object but only a subset of the fields will be populated.
	 * 
	 * @param g 
	 * @param callback Function to call when the API call completes
	 */
	void guild_get_preview(snowflake g, command_completion_event_t callback);

	/**
	 * @brief Get a guild member
	 * 
	 * @param guild_id 
	 * @param user_id 
	 * @param callback Function to call when the API call completes
	 */
	void guild_get_member(snowflake guild_id, snowflake user_id, command_completion_event_t callback);

	/**
	 * @brief Get all guild members
	 * 
	 * @param guild_id 
	 * @param callback Function to call when the API call completes
	 */
	void guild_get_members(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Add guild member. Needs a specific oauth2 scope, from which you get the access_token.
	 * 
	 * @param gm 
	 * @param access_token 
	 * @param callback Function to call when the API call completes 
	 */
	void guild_add_member(const guild_member& gm, const std::string &access_token, command_completion_event_t callback = {});

	/**
	 * @brief Edit the properties of an existing guild member
	 * 
	 * @param gm 
	 * @param callback Function to call when the API call completes
	 */
	void guild_edit_member(const guild_member& gm, command_completion_event_t callback = {});

	/**
	 * @brief Change current user nickname
	 * 
	 * @param guild_id 
	 * @param nickname 
	 * @param callback Function to call when the API call completes
	 */
	void guild_set_nickname(snowflake guild_id, const std::string &nickname, command_completion_event_t callback = {});

	/**
	 * @brief Add role to guild member
	 * 
	 * @param guild_id 
	 * @param user_id 
	 * @param role_id 
	 * @param callback Function to call when the API call completes
	 */
	void guild_member_add_role(snowflake guild_id, snowflake user_id, snowflake role_id, command_completion_event_t callback = {});

	/**
	 * @brief Remove role from guild member
	 * 
	 * @param guild_id 
	 * @param user_id 
	 * @param role_id 
	 * @param callback Function to call when the API call completes
	 */
	void guild_member_delete_role(snowflake guild_id, snowflake user_id, snowflake role_id, command_completion_event_t callback = {});

	/**
	 * @brief Remove (kick) a guild member
	 * 
	 * @param guild_id 
	 * @param user_id 
	 * @param callback Function to call when the API call completes
	 */
	void guild_member_delete(snowflake guild_id, snowflake user_id, command_completion_event_t callback = {});

	/**
	 * @brief Add guild ban
	 * 
	 * @param guild_id 
	 * @param user_id 
	 * @param delete_message_days 
	 * @param reason 
	 * @param callback Function to call when the API call completes
	 */
	void guild_ban_add(snowflake guild_id, snowflake user_id, uint32_t delete_message_days, const std::string &reason, command_completion_event_t callback = {});

	/**
	 * @brief Delete guild ban
	 * 
	 * @param guild_id 
	 * @param user_id 
	 * @param callback Function to call when the API call completes
	 */
	void guild_ban_delete(snowflake guild_id, snowflake user_id, command_completion_event_t callback = {});

	/**
	 * @brief Get guild ban list
	 * 
	 * @param guild_id 
	 * @param callback Function to call when the API call completes
	 */
	void guild_get_bans(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get single guild ban
	 * 
	 * @param guild_id 
	 * @param user_id 
	 * @param callback Function to call when the API call completes
	 */
	void guild_get_ban(snowflake guild_id, snowflake user_id, command_completion_event_t callback);

	/**
	 * @brief Get a template
	 * 
	 * @param code Template code
	 * @param callback Function to call when the API call completes
	 */
	void template_get(const std::string &code, command_completion_event_t callback);

	/**
	 * @brief Create a new guild based on a template.
	 * 
	 * @param code 
	 * @param name 
	 * @param callback Function to call when the API call completes
	 */
	void guild_create_from_template(const std::string &code, const std::string &name, command_completion_event_t callback = {});

	/**
	 * @brief Get guild templates
	 * 
	 * @param guild_id 
	 * @param callback Function to call when the API call completes
	 */
	void guild_templates_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Creates a template for the guild
	 * 
	 * @param guild_id 
	 * @param name 
	 * @param description 
	 * @param callback Function to call when the API call completes
	 */
	void guild_template_create(snowflake guild_id, const std::string &name, const std::string &description, command_completion_event_t callback);

	/**
	 * @brief Syncs the template to the guild's current state.
	 * 
	 * @param guild_id 
	 * @param code 
	 * @param callback Function to call when the API call completes
	 */
	void guild_template_sync(snowflake guild_id, const std::string &code, command_completion_event_t callback = {});

	/**
	 * @brief Modifies the template's metadata.
	 * 
	 * @param guild_id 
	 * @param code 
	 * @param name 
	 * @param description 
	 * @param callback Function to call when the API call completes
	 */
	void guild_template_modify(snowflake guild_id, const std::string &code, const std::string &name, const std::string &description, command_completion_event_t callback = {});

	/**
	 * @brief Deletes the template
	 * 
	 * @param guild_id 
	 * @param code 
	 * @param callback Function to call when the API call completes
	 */
	void guild_template_delete(snowflake guild_id, const std::string &code, command_completion_event_t callback = {});

	/**
	 * @brief Create a guild
	 * 
	 * @param g 
	 * @param callback Function to call when the API call completes
	 */
	void guild_create(const class guild &g, command_completion_event_t callback = {});

	/**
	 * @brief Edit a guild
	 * 
	 * @param g 
	 * @param callback Function to call when the API call completes
	 */
	void guild_edit(const class guild &g, command_completion_event_t callback = {});

	/**
	 * @brief Delete a guild
	 * 
	 * @param guild_id 
	 * @param callback Function to call when the API call completes
	 */
	void guild_delete(snowflake guild_id, command_completion_event_t callback = {});

	/**
	 * @brief Get all emojis for a guild
	 * 
	 * @param guild_id 
	 * @param callback Function to call when the API call completes
	 */
	void guild_emojis_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get a single emoji
	 * 
	 * @param guild_id 
	 * @param emoji_id 
	 * @param callback Function to call when the API call completes
	 */
	void guild_emoji_get(snowflake guild_id, snowflake emoji_id, command_completion_event_t callback);

	/**
	 * @brief Create single emoji.
	 * You must ensure that the emoji passed contained image data using the emoji::load_image() method.
	 * 
	 * @param guild_id 
	 * @param newemoji 
	 * @param callback Function to call when the API call completes
	 */
	void guild_emoji_create(snowflake guild_id, const class emoji& newemoji, command_completion_event_t callback = {});

	/**
	 * @brief Edit a single emoji.
	 * You must ensure that the emoji passed contained image data using the emoji::load_image() method.
	 * 
	 * @param guild_id 
	 * @param newemoji 
	 * @param callback Function to call when the API call completes
	 */
	void guild_emoji_edit(snowflake guild_id, const class emoji& newemoji, command_completion_event_t callback = {});

	/**
	 * @brief Delete a guild emoji
	 * 
	 * @param guild_id 
	 * @param emoji_id 
	 * @param callback Function to call when the API call completes
	 */
	void guild_emoji_delete(snowflake guild_id, snowflake emoji_id, command_completion_event_t callback = {});

	/**
	 * @brief Get prune counts
	 * 
	 * @param guild_id 
	 * @param pruneinfo 
	 * @param callback Function to call when the API call completes
	 */
	void guild_get_prune_counts(snowflake guild_id, const class prune& pruneinfo, command_completion_event_t callback);

	/**
	 * @brief Begin guild prune
	 * 
	 * @param guild_id 
	 * @param pruneinfo 
	 * @param callback Function to call when the API call completes
	 */
	void guild_begin_prune(snowflake guild_id, const class prune& pruneinfo, command_completion_event_t callback = {});

	/**
	 * @brief Get guild voice regions
	 * 
	 * @param guild_id 
	 * @param callback Function to call when the API call completes
	 */
	void guild_get_voice_regions(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get the guild invites objectGet guild invites
	 * 
	 * @param guild_id 
	 * @param callback Function to call when the API call completes
	 */
	void get_guild_invites(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get guild itegrations
	 * 
	 * @param guild_id 
	 * @param callback Function to call when the API call completes
	 */
	void guild_get_integrations(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Modify guild integration
	 * 
	 * @param guild_id 
	 * @param i 
	 * @param callback Function to call when the API call completes
	 */
	void guild_modify_integration(snowflake guild_id, const class integration &i, command_completion_event_t callback = {}); 

	/**
	 * @brief Delete guild integration
	 * 
	 * @param guild_id 
	 * @param integration_id 
	 * @param callback Function to call when the API call completes
	 */
	void guild_delete_integration(snowflake guild_id, snowflake integration_id, command_completion_event_t callback = {});

	/**
	 * @brief Sync guild integration
	 * 
	 * @param guild_id 
	 * @param integration_id 
	 * @param callback Function to call when the API call completes
	 */
	void guild_sync_integration(snowflake guild_id, snowflake integration_id, command_completion_event_t callback = {});

	/**
	 * @brief Get guild widget
	 * 
	 * @param guild_id 
	 * @param callback Function to call when the API call completes
	 */
	void guild_get_widget(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Edit guild widget
	 * 
	 * @param guild_id 
	 * @param gw 
	 * @param callback Function to call when the API call completes
	 */
	void guild_edit_widget(snowflake guild_id, const class guild_widget &gw, command_completion_event_t callback = {});

	/**
	 * @brief Get guild vanity url, if enabled
	 * 
	 * @param guild_id 
	 * @param callback Function to call when the API call completes
	 */
	void guild_get_vanity(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Create a webhook
	 * 
	 * @param w 
	 * @param callback Function to call when the API call completes
	 */
	void create_webhook(const class webhook &w, command_completion_event_t callback = {});

	/**
	 * @brief Get the guild webhooks objectGet guild webhooks
	 * 
	 * @param guild_id 
	 * @param callback Function to call when the API call completes
	 */
	void get_guild_webhooks(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Get the channel webhooks objectGet channel webhooks
	 * 
	 * @param channel_id 
	 * @param callback Function to call when the API call completes
	 */
	void get_channel_webhooks(snowflake channel_id, command_completion_event_t callback);

	/**
	 * @brief Get the webhook objectGet webhook
	 * 
	 * @param webhook_id 
	 * @param callback Function to call when the API call completes
	 */
	void get_webhook(snowflake webhook_id, command_completion_event_t callback);

	/**
	 * @brief Get the webhook with token objectGet webhook using token
	 * 
	 * @param webhook_id 
	 * @param token 
	 * @param callback Function to call when the API call completes
	 */
	void get_webhook_with_token(snowflake webhook_id, const std::string &token, command_completion_event_t callback);

	/**
	 * @brief Edit webhook
	 * 
	 * @param wh 
	 * @param callback Function to call when the API call completes
	 */
	void edit_webhook(const class webhook& wh, command_completion_event_t callback = {});

	/**
	 * @brief Edit webhook with token (token is encapsulated in the webhook object)
	 * 
	 * @param wh 
	 * @param callback Function to call when the API call completes
	 */
	void edit_webhook_with_token(const class webhook& wh, command_completion_event_t callback = {});

	/**
	 * @brief Delete a webhook
	 * 
	 * @param webhook_id 
	 * @param callback Function to call when the API call completes
	 */
	void delete_webhook(snowflake webhook_id, command_completion_event_t callback = {});

	/**
	 * @brief Delete webhook with token
	 * 
	 * @param webhook_id 
	 * @param token 
	 * @param callback Function to call when the API call completes
	 */
	void delete_webhook_with_token(snowflake webhook_id, const std::string &token, command_completion_event_t callback = {});

	/**
	 * @brief Execute webhook
	 * 
	 * @param wh 
	 * @param m 
	 * @param callback Function to call when the API call completes
	 */
	void execute_webhook(const class webhook &wh, const class message &m, command_completion_event_t callback = {});

	/**
	 * @brief Edit webhook message
	 * 
	 * @param wh 
	 * @param m 
	 * @param callback Function to call when the API call completes
	 */
	void edit_webhook_message(const class webhook &wh, const class message &m, command_completion_event_t callback = {});

	/**
	 * @brief Delete webhook message
	 * 
	 * @param wh 
	 * @param message_id 
	 * @param callback Function to call when the API call completes
	 */
	void delete_webhook_message(const class webhook &wh, snowflake message_id, command_completion_event_t callback = {});

	/**
	 * @brief Get a role for a guild
	 * 
	 * @param guild_id 
	 * @param callback Function to call when the API call completes
	 */
	void roles_get(snowflake guild_id, command_completion_event_t callback);

	/**
	 * @brief Create a role on a guild
	 * 
	 * @param r 
	 * @param callback Function to call when the API call completes
	 */
	void role_create(const class role &r, command_completion_event_t callback = {});

	/**
	 * @brief Edit a role on a guild
	 * 
	 * @param r 
	 * @param callback Function to call when the API call completes
	 */
	void role_edit(const class role &r, command_completion_event_t callback = {});

	/**
	 * @brief Edit a role's position in a guild
	 * 
	 * @param r 
	 * @param callback Function to call when the API call completes
	 */
	void role_edit_position(const class role &r, command_completion_event_t callback = {});

	/**
	 * @brief Delete a role
	 * 
	 * @param guild_id 
	 * @param role_id 
	 * @param callback Function to call when the API call completes
	 */
	void role_delete(snowflake guild_id, snowflake role_id, command_completion_event_t callback = {});

	/**
	 * @brief Get a user by id
	 * 
	 * @param user_id 
	 * @param callback Function to call when the API call completes
	 */
	void user_get(snowflake user_id, command_completion_event_t callback);

	/**
	 * @brief Get current (bot) user
	 * 
	 * @param callback Function to call when the API call completes
	 */
	void current_user_get(command_completion_event_t callback);

	/**
	 * @brief Get current (bot) user guilds
	 * 
	 * @param callback Function to call when the API call completes
	 */
	void current_user_get_guilds(command_completion_event_t callback);

	/**
	 * @brief Edit current (bot) user
	 * 
	 * @param nickname 
	 * @param image_blob 
	 * @param type 
	 * @param callback Function to call when the API call completes
	 */
	void current_user_edit(const std::string &nickname, const std::string& image_blob, image_type type, command_completion_event_t callback = {});

	/**
	 * @brief Get current user DM channels
	 * 
	 * @param callback Function to call when the API call completes
	 */
	void current_user_get_dms(command_completion_event_t callback);

	/**
	 * @brief Create a dm channel
	 * 
	 * @param user_id 
	 * @param callback Function to call when the API call completes
	 */
	void create_dm_channel(snowflake user_id, command_completion_event_t callback = {});

	/**
	 * @brief Leave a guild
	 * 
	 * @param guild_id 
	 * @param callback Function to call when the API call completes
	 */
	void current_user_leave_guild(snowflake guild_id, command_completion_event_t callback = {});

	/**
	 * @brief Get all voice regions
	 * 
	 * @param callback Function to call when the API call completes
	 */
	void get_voice_regions(command_completion_event_t callback);

	/**
	 * @brief Get the gateway information for the bot using the token
	 * 
	 * @param callback Function to call when the API call completes
	 */
	void get_gateway_bot(command_completion_event_t callback);


};

};
