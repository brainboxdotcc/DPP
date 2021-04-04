#pragma once

#include <string>
#include <map>
#include <variant>
#include <dpp/discord.h>
#include <dpp/dispatcher.h>
#include <dpp/json_fwd.hpp>
#include <spdlog/fwd.h>
#include <dpp/discordclient.h>
#include <dpp/queues.h>

using  json = nlohmann::json;

namespace dpp {

struct confirmation {
};

typedef std::variant<
                confirmation,
                message,
                user,
                guild_member,
                channel,
                guild,
                role
        > confirmable_t;

struct confirmation_callback_t {
	std::string type;
	http_request_completion_t http_info;
	confirmable_t value;

	confirmation_callback_t() = default;
	confirmation_callback_t(const std::string &_type, const confirmable_t& _value, const http_request_completion_t& _http);
};

typedef std::function<void(const confirmation_callback_t&)> command_completion_event_t;
typedef std::function<void(json&, const http_request_completion_t&)> json_encode_t;

/** The cluster class represents a group of shards and a command queue for sending and 
 * receiving commands from discord via HTTP. You should usually instantiate a cluster object
 * at the very least to make use of the library.
 */
class cluster {
	/** queue system for commands sent to Discord, and any replies */
	request_queue* rest;
public:
	/** Current bot token for all shards on this cluster and all commands sent via HTTP */
	std::string token;

	/** Current bitmask of gateway intents */
	uint32_t intents;

	/** Total number of shards across all clusters */
	uint32_t numshards;

	/** ID of this cluster, between 0 and MAXCLUSTERS-1 inclusive */
	uint32_t cluster_id;

	/** Total number of clusters that are active */
	uint32_t maxclusters;

	/** Optional spdlog::logger log object */
	spdlog::logger* log;

	/** Routes events from Discord back to user program code via std::functions */
	dpp::dispatcher dispatch;

	/** Active shards on this cluster. Shard IDs may have gaps between if there 
	 * are multiple clusters.
	 */
	std::map<uint32_t, class DiscordClient*> shards;

	/** Constructor for creating a cluster. All but the token are optional.
	 * @param token The bot token to use for all HTTP commands and websocket connections
	 * @param intents A bitmask of dpd::intents values for all shards on this cluster. This is required to be sent for all bots with over 100 servers.
	 * @param shards The total number of shards on this bot. If there are multiple clusters, then (shards / clusters) actual shards will run on this cluster.
	 * @param cluster_id The ID of this cluster, should be between 0 and MAXCLUSTERS-1
	 * @param maxclusters The total number of clusters that are active, which may be on seperate processes or even separate machines.
	 * @param log An optional spdlog::logger object for logging details about the cluster
	 */
	cluster(const std::string &token, uint32_t intents = 0, uint32_t shards = 1, uint32_t cluster_id = 0, uint32_t maxclusters = 1, spdlog::logger* log = nullptr);

	/** Destructor */
	~cluster();

	/** Start the cluster, connecting all its shards.
	 * Returns once all shards are connected.
	 */
	void start();

	/* Functions for attaching to event handlers */

	/** Called for VOICE_STATE_UPDATE */
	void on_voice_state_update (std::function<void(const voice_state_update_t& _event)> _voice_state_update);

	/** Called for ON_INTERACTION_CREATE */
	void on_interaction_create (std::function<void(const interaction_create_t& _event)> _interaction_create);
	void on_guild_delete (std::function<void(const guild_delete_t& _event)> _guild_delete);
	void on_channel_delete (std::function<void(const channel_delete_t& _event)> _channel_delete);
	void on_channel_update (std::function<void(const channel_update_t& _event)> _channel_update);
	void on_ready (std::function<void(const ready_t& _event)> _ready);
	void on_message_delete (std::function<void(const message_delete_t& _event)> _message_delete);
	void on_application_command_delete (std::function<void(const application_command_delete_t& _event)> _application_command_delete);
	void on_guild_member_remove (std::function<void(const guild_member_remove_t& _event)> _guild_member_remove);
	void on_application_command_create (std::function<void(const application_command_create_t& _event)> _application_command_create);
	void on_resumed (std::function<void(const resumed_t& _event)> _resumed);
	void on_guild_role_create (std::function<void(const guild_role_create_t& _event)> _guild_role_create);
	void on_typing_start (std::function<void(const typing_start_t& _event)> _typing_start);
	void on_message_reaction_add (std::function<void(const message_reaction_add_t& _event)> _message_reaction_add);
	void on_guild_members_chunk (std::function<void(const guild_members_chunk_t& _event)> _guild_members_chunk);
	void on_message_reaction_remove (std::function<void(const message_reaction_remove_t& _event)> _message_reaction_remove);
	void on_guild_create (std::function<void(const guild_create_t& _event)> _guild_create);
	void on_channel_create (std::function<void(const channel_create_t& _event)> _channel_create);
	void on_message_reaction_remove_emoji (std::function<void(const message_reaction_remove_emoji_t& _event)> _message_reaction_remove_emoji);
	void on_message_delete_bulk (std::function<void(const message_delete_bulk_t& _event)> _message_delete_bulk);
	void on_guild_role_update (std::function<void(const guild_role_update_t& _event)> _guild_role_update);
	void on_guild_role_delete (std::function<void(const guild_role_delete_t& _event)> _guild_role_delete);
	void on_channel_pins_update (std::function<void(const channel_pins_update_t& _event)> _channel_pins_update);
	void on_message_reaction_remove_all (std::function<void(const message_reaction_remove_all_t& _event)> _message_reaction_remove_all);
	void on_voice_server_update (std::function<void(const voice_server_update_t& _event)> _voice_server_update);
	void on_guild_emojis_update (std::function<void(const guild_emojis_update_t& _event)> _guild_emojis_update);
	void on_presence_update (std::function<void(const presence_update_t& _event)> _presence_update);
	void on_webhooks_update (std::function<void(const webhooks_update_t& _event)> _webhooks_update);
	void on_guild_member_add (std::function<void(const guild_member_add_t& _event)> _guild_member_add);
	void on_invite_delete (std::function<void(const invite_delete_t& _event)> _invite_delete);
	void on_guild_update (std::function<void(const guild_update_t& _event)> _guild_update);
	void on_guild_integrations_update (std::function<void(const guild_integrations_update_t& _event)> _guild_integrations_update);
	void on_guild_member_update (std::function<void(const guild_member_update_t& _event)> _guild_member_update);
	void on_application_command_update (std::function<void(const application_command_update_t& _event)> _application_command_update);
	void on_invite_create (std::function<void(const invite_create_t& _event)> _invite_create);
	void on_message_update (std::function<void(const message_update_t& _event)> _message_update);
	void on_user_update (std::function<void(const user_update_t& _event)> _user_update);
	void on_message_create (std::function<void(const message_create_t& _event)> _message_create);
	void on_guild_ban_add (std::function<void(const guild_ban_add_t& _event)> _guild_ban_add);
	void on_integration_create (std::function<void(const integration_create_t& _event)> _integration_create);
	void on_integration_update (std::function<void(const integration_update_t& _event)> _integration_update);
	void on_integration_delete (std::function<void(const integration_delete_t& _event)> _integration_delete);

	/* Post a REST request. Where possible use a helper method instead like message_create */
	void post_rest(const std::string &endpoint, const std::string &parameters, http_method method, const std::string &postdata, json_encode_t callback);

	/* Send a message to a channel. The callback function is called when the message has been sent */
	void message_create(const struct message &m, command_completion_event_t callback);

	/* Edit a message on a channel. The callback function is called when the message has been edited */
	void message_edit(const struct message &m, command_completion_event_t callback);

	/* Delete a message from a channel. The callback function is called when the message has been edited */
	void message_delete(snowflake message_id, snowflake channel_id, command_completion_event_t callback);

	/* Create a channel */
	void channel_create(const class channel &c, command_completion_event_t callback);

	/* Edit a channel. */
	void channel_edit(const class channel &c, command_completion_event_t callback);
	void channel_edit_position(const class channel &c, command_completion_event_t callback);

	/* Delete a channel */
	void channel_delete(snowflake channel_id, command_completion_event_t callback);

	/* Create a guild */
	void guild_create(const class guild &g, command_completion_event_t callback);

	/* Edit a guild */
	void guild_edit(const class guild &g, command_completion_event_t callback);

	/* Delete a guild */
	void guild_delete(snowflake guild_id, command_completion_event_t callback);

	/* Create a role */
	void role_create(const class role &r, command_completion_event_t callback);

        /* Edit a role */
	void role_edit(const class role &r, command_completion_event_t callback);
	void role_edit_position(const class role &r, command_completion_event_t callback);

	/* Delete a role */
	void role_delete(snowflake guild_id, snowflake role_id, command_completion_event_t callback);


};

};
