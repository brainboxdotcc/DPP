#pragma once

#include <dpp/discord.h>
#include <dpp/json_fwd.hpp>

#define event_decl(x) class x : public event { public: virtual void handle(class DiscordClient* client, nlohmann::json &j); };

class event {
public:
	virtual void handle(class DiscordClient* client, nlohmann::json &j) = 0;
};

event_decl(guild_create);
event_decl(guild_update);
event_decl(guild_delete);
event_decl(guild_ban_add);
event_decl(guild_ban_remove);
event_decl(guild_emojis_update);
event_decl(guild_integrations_update);
event_decl(guild_member_add);
event_decl(guild_member_remove);
event_decl(guild_members_chunk);
event_decl(guild_member_update);

event_decl(guild_role_create);
event_decl(guild_role_update);
event_decl(guild_role_delete);

event_decl(resumed);
event_decl(ready);

event_decl(channel_create);
event_decl(channel_update);
event_decl(channel_delete);
event_decl(channel_pins_update);

event_decl(message_create);
event_decl(message_update);
event_decl(message_delete);
event_decl(message_delete_bulk);

event_decl(presence_update);
event_decl(typing_start);

event_decl(user_update);

event_decl(message_reaction_add);
event_decl(message_reaction_remove);
event_decl(message_reaction_remove_all);

event_decl(voice_state_update);
event_decl(voice_server_update);

event_decl(webhooks_update);

