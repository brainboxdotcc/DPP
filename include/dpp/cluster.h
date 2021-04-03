#pragma once

#include <string>
#include <map>
#include <dpp/discord.h>
#include <dpp/dispatcher.h>
#include <spdlog/fwd.h>
#include <dpp/discordclient.h>
#include <dpp/queues.h>

namespace dpp {

class cluster {
	request_queue* rest;
public:
	std::string token;
	uint32_t intents;
	uint32_t numshards;
	uint32_t cluster_id;
	uint32_t maxclusters;
	spdlog::logger* log;
	dpp::dispatcher dispatch;
	std::map<uint32_t, class DiscordClient*> shards;

	cluster(const std::string &token, uint32_t intents = 0, uint32_t shards = 1, uint32_t cluster_id = 0, uint32_t maxclusters = 1, spdlog::logger* log = nullptr);
	~cluster();
	void start();

	/* Functions for attaching to event handlers */
	void on_voice_state_update (std::function<void(const voice_state_update_t& _event)> _voice_state_update);
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
	void post_rest(const std::string &endpoint, const std::string &parameters, http_method method, const std::string &postdata, http_completion_event callback);

	/* Send a message to a channel. The callback function is called when the message has been sent */
	void message_create(const struct message &m, http_completion_event callback);

	/* Edit a message on a channel. The callback function is called when the message has been edited */
	void message_edit(const struct message &m, http_completion_event callback);

	/* Delete a message from a channel. The callback function is called when the message has been edited */
	void message_delete(snowflake message_id, snowflake channel_id, http_completion_event callback);

	/* Create a channel */
	void channel_create(const class channel &c, http_completion_event callback);

	/* Edit a channel. */
	void channel_edit(const class channel &c, http_completion_event callback);
	void channel_edit_position(const class channel &c, http_completion_event callback);

	/* Delete a channel */
	void channel_delete(snowflake channel_id, http_completion_event callback);

	/* Create a guild */
	void guild_create(const class guild &g, http_completion_event callback);

	/* Edit a guild */
	void guild_edit(const class guild &g, http_completion_event callback);

	/* Delete a guild */
	void guild_delete(snowflake guild_id, http_completion_event callback);

	/* Create a role */
	void role_create(const class role &r, http_completion_event callback);

        /* Edit a role */
	void role_edit(const class role &r, http_completion_event callback);
	void role_edit_position(const class role &r, http_completion_event callback);

	/* Delete a role */
	void role_delete(snowflake guild_id, snowflake role_id, http_completion_event callback);


};

};
