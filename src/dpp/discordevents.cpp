#include <string>
#include <iostream>
#include <fstream>
#include <dpp/discordclient.h>
#include <dpp/discord.h>
#include <dpp/event.h>
#include <dpp/cache.h>
#include <dpp/stringops.h>
#include <spdlog/spdlog.h>

uint64_t SnowflakeNotNull(json* j, const char *keyname)
{
	return j->find(keyname) != j->end() && !(*j)[keyname].is_null() ? from_string<uint64_t>((*j)[keyname].get<std::string>(), std::dec) : 0;
}

std::string StringNotNull(json* j, const char *keyname)
{
	return j->find(keyname) != j->end() && !(*j)[keyname].is_null() ? (*j)[keyname].get<std::string>() : "";
}

uint32_t Int32NotNull(json* j, const char *keyname)
{
	return j->find(keyname) != j->end() && !(*j)[keyname].is_null() ? (*j)[keyname].get<uint32_t>() : 0;
}

uint16_t Int16NotNull(json* j, const char *keyname)
{
	return j->find(keyname) != j->end() && !(*j)[keyname].is_null() ? (*j)[keyname].get<uint16_t>() : 0;
}

uint8_t Int8NotNull(json* j, const char *keyname)
{
	return j->find(keyname) != j->end() && !(*j)[keyname].is_null() ? (*j)[keyname].get<uint8_t>() : 0;
}

bool BoolNotNull(json* j, const char *keyname)
{
	return (j->find(keyname) != j->end() && !(*j)[keyname].is_null() && (*j)[keyname].get<bool>() == true);
}

std::map<std::string, event*> events = {
	{ "GUILD_CREATE", new guild_create() },
	{ "GUILD_UPDATE", new guild_update() },
	{ "GUILD_DELETE", new guild_delete() },
	{ "GUILD_MEMBER_UPDATE", new guild_member_update() },
	{ "RESUMED", new resumed() },
	{ "READY", new ready() },
	{ "CHANNEL_CREATE", new channel_create() },
	{ "CHANNEL_UPDATE", new channel_update() },
	{ "CHANNEL_DELETE", new channel_delete() },
	{ "PRESENCE_UPDATE", new presence_update() },
	{ "TYPING_START", new typing_start() },
	{ "MESSAGE_CREATE", new message_create() },
	{ "MESSAGE_UPDATE", new message_update() },
	{ "MESSAGE_DELETE", new message_delete() },
	{ "MESSAGE_REACTION_ADD", new message_reaction_add() },
	{ "MESSAGE_REACTION_REMOVE", new message_reaction_remove() },
	{ "MESSAGE_REACTION_REMOVE_ALL", new message_reaction_remove_all() },
	{ "CHANNEL_PINS_UPDATE", new channel_pins_update() },
	{ "GUILD_BAN_ADD", new guild_ban_add() },
	{ "GUILD_EMOJIS_UPDATE", new guild_emojis_update() },
	{ "GUILD_INTEGRATIONS_UPDATE", new guild_integrations_update() },
	{ "GUILD_MEMBER_ADD", new guild_member_add() },
	{ "GUILD_MEMBER_REMOVE", new guild_member_remove() },
	{ "GUILD_MEMBERS_CHUNK", new guild_members_chunk() },
	{ "GUILD_ROLE_CREATE", new guild_role_create() },
	{ "GUILD_ROLE_UPDATE", new guild_role_update() },
	{ "GUILD_ROLE_DELETE", new guild_role_delete() },
	{ "VOICE_STATE_UPDATE", new voice_state_update() },
	{ "VOICE_SERVER_UPDATE", new voice_server_update() },
	{ "WEBHOOKS_UPDATE", new webhooks_update() }
};

void DiscordClient::HandleEvent(const std::string &event, json &j)
{
	auto ev_iter = events.find(event);
	if (ev_iter != events.end()) {
		ev_iter->second->handle(this, j);
	} else {
		logger->debug("Unhandled event: {}", event);
	}
}
