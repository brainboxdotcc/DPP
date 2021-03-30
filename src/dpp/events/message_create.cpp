#include <dpp/discord.h>
#include <dpp/event.h>
#include <string>
#include <iostream>
#include <fstream>
#include <dpp/discordclient.h>
#include <dpp/discord.h>
#include <dpp/cache.h>
#include <dpp/stringops.h>
#include <nlohmann/json.hpp>
#include <dpp/dispatcher.h>
#include <dpp/discordevents.h>

using json = nlohmann::json;

void message_create::handle(class DiscordClient* client, json &j) {

	json d = j["d"];
	dpp::message_create_t msg;
	dpp::message m;
	
	msg.msg = &m;

	m.id = SnowflakeNotNull(&d, "id");			
	m.channel_id = SnowflakeNotNull(&d, "channel_id");
	m.guild_id = SnowflakeNotNull(&d, "guild_id");
	/* May be null, if its null cache it from the partial */
	if (d.find("author") != d.end()) {
		json &author = d["author"];
		m.author = dpp::find_user(SnowflakeNotNull(&author, "id"));
	}
	/* TODO: Fill this in */
	dpp::guild* g = dpp::find_guild(m.guild_id);
	m.member = nullptr;
	if (g && d.find("member") != d.end()) {
		json& mi = d["member"];
		dpp::snowflake uid = SnowflakeNotNull(&mi, "id");
		auto thismember = g->members.find(uid);
		if (thismember != g->members.end()) {
			m.member = thismember->second;
		}
	}
	m.content = StringNotNull(&d, "content");
	/* TODO: Waiting on TimestampNotNull */
	/*m.sent, m.edited */
	m.tts = BoolNotNull(&d, "tts");
	m. mention_everyone = BoolNotNull(&d, "mention_everyone");
	/* TODO: Fix these */
	m.mentions = nullptr;
	m.mention_roles = nullptr;
	/* TODO: Populate these */
	/* m.mention_channels, m.attachments, m.embeds, m.reactions */
	m.nonce = StringNotNull(&d, "nonce");
	m.pinned = BoolNotNull(&d, "pinned");
	m.webhook_id = SnowflakeNotNull(&d, "webhook_id");

	client->creator->dispatch.message_create(msg);
}

