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
#include <dpp/snowflake.h>
#include <dpp/json_fwd.hpp>

#define event_decl(x) class x : public event { public: virtual void handle(dpp::discord_client* client, nlohmann::json &j, const std::string &raw); };

namespace dpp { 

class discord_client;

/**
 * @brief The events namespace holds the internal event handlers for each websocket event.
 * These are handled internally and also dispatched to the user code if the event is hooked.
 */
namespace events {

/**
 * @brief An event object represents an event handled internally, passed from the websocket e.g. MESSAGE_CREATE.
 */
class DPP_EXPORT event {
public:
	/** Pure virtual method for event handler code
	 * @param client The creating shard
	 * @param j The json data of the event
	 * @param raw The raw event json
	 */
	virtual void handle(class discord_client* client, nlohmann::json &j, const std::string &raw) = 0;
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
event_decl(guild_join_request_delete);
event_decl(guild_stickers_update);

/* Stage channels */
event_decl(stage_instance_create);
event_decl(stage_instance_update);
event_decl(stage_instance_delete);

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

/* Threads */
event_decl(thread_create);
event_decl(thread_update);
event_decl(thread_delete);
event_decl(thread_list_sync);
event_decl(thread_member_update);
event_decl(thread_members_update);

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

/* Application commands */
event_decl(interaction_create);

/* Integrations */
event_decl(integration_create);
event_decl(integration_update);
event_decl(integration_delete);

/* Scheduled events */
event_decl(guild_scheduled_event_create);
event_decl(guild_scheduled_event_update);
event_decl(guild_scheduled_event_delete);
event_decl(guild_scheduled_event_user_add);
event_decl(guild_scheduled_event_user_remove);

}};
