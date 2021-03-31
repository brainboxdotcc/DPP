#define _XOPEN_SOURCE
#include <string>
#include <iostream>
#include <fstream>
#include <time.h>
#include <dpp/discordclient.h>
#include <dpp/discord.h>
#include <dpp/event.h>
#include <dpp/cache.h>
#include <dpp/stringops.h>
#include <spdlog/spdlog.h>

uint64_t SnowflakeNotNull(json* j, const char *keyname)
{
	return j->find(keyname) != j->end() && !(*j)[keyname].is_null() && (*j)[keyname].is_string() ? from_string<uint64_t>((*j)[keyname].get<std::string>(), std::dec) : 0;
}

std::string StringNotNull(json* j, const char *keyname)
{
	return j->find(keyname) != j->end() && !(*j)[keyname].is_null() && (*j)[keyname].is_string() ? (*j)[keyname].get<std::string>() : "";
}

uint32_t Int32NotNull(json* j, const char *keyname)
{
	return j->find(keyname) != j->end() && !(*j)[keyname].is_null() && !(*j)[keyname].is_string() ? (*j)[keyname].get<uint32_t>() : 0;
}

uint16_t Int16NotNull(json* j, const char *keyname)
{
	return j->find(keyname) != j->end() && !(*j)[keyname].is_null() && !(*j)[keyname].is_string() ? (*j)[keyname].get<uint16_t>() : 0;
}

uint8_t Int8NotNull(json* j, const char *keyname)
{
	return j->find(keyname) != j->end() && !(*j)[keyname].is_null() && !(*j)[keyname].is_string() ? (*j)[keyname].get<uint8_t>() : 0;
}

bool BoolNotNull(json* j, const char *keyname)
{
	return (j->find(keyname) != j->end() && !(*j)[keyname].is_null() && (*j)[keyname].get<bool>() == true);
}

time_t TimestampNotNull(json* j, const char* keyname)
{
	/* Parses discord ISO 8061 timestamps to time_t, accounting for local time adjustment.
	 * Note that discord timestamps contain a decimal seconds part, which time_t and struct tm
	 * can't handle. We strip these out.
	 */
	time_t retval = 0;
	if (j->find(keyname) != j->end() && !(*j)[keyname].is_null() && (*j)[keyname].is_string()) {
		tm timestamp;
		std::string timedate = (*j)[keyname].get<std::string>();
		std::string tzpart = timedate.substr(timedate.find('+'), timedate.length());
		timedate = timedate.substr(0, timedate.find('.')) + tzpart ;
		strptime(timedate.substr(0, 19).c_str(), "%FT%TZ%z", &timestamp);
		timestamp.tm_isdst = 0;
		retval = mktime(&timestamp);
	}
	return retval;
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
	{ "MESSAGE_REACTION_REMOVE_EMOJI", new message_reaction_remove_emoji() },
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
	{ "WEBHOOKS_UPDATE", new webhooks_update() },
	{ "INVITE_CREATE", new invite_create() },
	{ "INVITE_DELETE", new invite_delete() },
	{ "APPLICATION_COMMAND_CREATE", new application_command_create() },
	{ "APPLICATION_COMMAND_UPDATE", new application_command_update() },
	{ "APPLICATION_COMMAND_DELETE", new application_command_delete() },
	{ "INTERACTION_CREATE", new interaction_create() }
};

void DiscordClient::HandleEvent(const std::string &event, json &j)
{
	auto ev_iter = events.find(event);
	if (ev_iter != events.end()) {
		ev_iter->second->handle(this, j);
	} else {
		logger->debug("Unhandled event: {}, {}", event, j.dump());
	}
}

void DiscordClient::add_chunk_queue(uint64_t id)
{
	chunk_queue.push(id);
}

