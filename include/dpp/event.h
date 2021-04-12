#pragma once

#include <dpp/discord.h>
#include <dpp/json_fwd.hpp>

#define event_decl(x) class x : public event { public: virtual void handle(dpp::DiscordClient* client, nlohmann::json &j, const std::string &raw); };

namespace dpp { 

class DiscordClient;

/**
 * @brief The events namespace holds the internal event handlers for each websocket event.
 * These are handled internally and also dispatched to the user code if the event is hooked.
 */
namespace events {

/** An event object represents an event handled internally, passed from the websocket e.g. MESSAGE_CREATE.
 */
class event {
public:
	/** Pure virtual method for event handler code
	 * @param client The creating shard
	 * @param j The json data of the event
	 * @param raw The raw event json
	 */
	virtual void handle(class DiscordClient* client, nlohmann::json &j, const std::string &raw) = 0;
};

/* Internal logger */
event_decl(logger);

/* Guilds */
event_decl(guild_create);
event_decl(guild_update);
event_decl(guild_delete);
event_decl(guild_ban_add);
event_decl(guild_ban_remove);
event_decl(guild_emojis_update);
event_decl(guild_integrations_update);

/* Guild members */
event_decl(guild_member_add);
event_decl(guild_member_remove);
event_decl(guild_members_chunk);
event_decl(guild_member_update);

/* Guild roles */
event_decl(guild_role_create);
event_decl(guild_role_update);
event_decl(guild_role_delete);

/* Session state */
event_decl(resumed);
event_decl(ready);

/* Channels */
event_decl(channel_create);
event_decl(channel_update);
event_decl(channel_delete);
event_decl(channel_pins_update);

/* Messages */
event_decl(message_create);
event_decl(message_update);
event_decl(message_delete);
event_decl(message_delete_bulk);

/* Presence/typing */
event_decl(presence_update);
event_decl(typing_start);

/* Users (outside of guild) */
event_decl(user_update);

/* Message reactions */
event_decl(message_reaction_add);
event_decl(message_reaction_remove);
event_decl(message_reaction_remove_all);
event_decl(message_reaction_remove_emoji);

/* Invites */
event_decl(invite_create);
event_decl(invite_delete);

/* Voice */
event_decl(voice_state_update);
event_decl(voice_server_update);

/* Webhooks */
event_decl(webhooks_update);

/* Slash commands */
event_decl(application_command_create);
event_decl(application_command_update);
event_decl(application_command_delete);
event_decl(interaction_create);

/* Integrations */
event_decl(integration_create);
event_decl(integration_update);
event_decl(integration_delete);

}};