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

event_decl(guild_member_update);

event_decl(resumed);
event_decl(ready);

event_decl(channel_create);
event_decl(channel_update);
event_decl(channel_delete);

