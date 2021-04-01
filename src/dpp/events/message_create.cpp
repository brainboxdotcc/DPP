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
	m.author = nullptr;
	dpp::user* authoruser = nullptr;
	/* May be null, if its null cache it from the partial */
	if (d.find("author") != d.end()) {
		json &author = d["author"];
		authoruser = dpp::find_user(SnowflakeNotNull(&author, "id"));
		if (!authoruser) {
			/* User does not exist yet, cache the partial as a user record */
			authoruser = new dpp::user();
			authoruser->fill_from_json(&author);
			dpp::get_user_cache()->store(authoruser);
		}
		m.author = authoruser;
	}
	/* Fill in member record, cache uncached ones */
	dpp::guild* g = dpp::find_guild(m.guild_id);
	m.member = nullptr;
	if (g && d.find("member") != d.end()) {
		json& mi = d["member"];
		dpp::snowflake uid = SnowflakeNotNull(&mi, "id");
		auto thismember = g->members.find(uid);
		if (thismember == g->members.end()) {
			if (authoruser) {
				dpp::guild_member* gm = new dpp::guild_member();
				gm->fill_from_json(&mi, g, authoruser);
				g->members[authoruser->id] = gm;
				m.member = gm;
			}
		} else {
			m.member = thismember->second;
		}
	}
	if (d.find("embeds") != d.end()) {
		json & el = d["embeds"];
		for (auto& e : el) {
			m.embeds.push_back(dpp::embed(&e));
		}
	}
	m.content = StringNotNull(&d, "content");
	m.sent = TimestampNotNull(&d, "timestamp");
	m.edited = TimestampNotNull(&d, "edited_timestamp");
	m.tts = BoolNotNull(&d, "tts");
	m. mention_everyone = BoolNotNull(&d, "mention_everyone");
	/* TODO: Fix these */
	m.mentions = nullptr;
	m.mention_roles = nullptr;
	/* TODO: Populate these */
	/* m.mention_channels, m.attachments, m.embeds, m.reactions */
	if (d["nonce"].is_string()) {
		m.nonce = StringNotNull(&d, "nonce");
	} else {
		m.nonce = std::to_string(SnowflakeNotNull(&d, "nonce"));
	}
	m.pinned = BoolNotNull(&d, "pinned");
	m.webhook_id = SnowflakeNotNull(&d, "webhook_id");
	if (client->creator->dispatch.message_create)
		client->creator->dispatch.message_create(msg);
}

