#include <string>
#include <iostream>
#include <fstream>
#include <dpp/discordclient.h>
#include <dpp/discord.h>
#include <dpp/cache.h>
#include <dpp/stringops.h>
#include <spdlog/spdlog.h>


std::map<std::string, dpp::guild_flags> featuremap = {
	{"INVITE_SPLASH", dpp::g_invite_splash },
	{"VIP_REGIONS", dpp::g_vip_regions },
	{"VANITY_URL", dpp::g_vanity_url },
	{"VERIFIED", dpp::g_verified },
	{"PARTNERED", dpp::g_partnered },
	{"COMMUNITY", dpp::g_community },
	{"COMMERCE", dpp::g_commerce },
	{"NEWS", dpp::g_news },
	{"DISCOVERABLE", dpp::g_discoverable },
	{"FEATUREABLE", dpp::g_featureable },
	{"ANIMATED_ICON", dpp::g_animated_icon },
	{"BANNER", dpp::g_banner },
	{"WELCOME_SCREEN_ENABLED", dpp::g_welcome_screen_enabled },
	{"MEMBER_VERIFICATION_GATE_ENABLED", dpp::g_member_verification_gate },
	{"PREVIEW_ENABLED", dpp::g_preview_enabled }
};

std::map<std::string, dpp::region> regionmap = {
        { "brazil", dpp::r_brazil },
        { "central-europe", dpp::r_central_europe },
        { "hong-kong", dpp::r_hong_kong },
        { "india", dpp::r_india },
        { "japan",  dpp::r_japan },
        { "russia", dpp::r_russia },
        { "singapore", dpp::r_singapore },
        { "south-africa", dpp::r_south_africa },
        { "sydney", dpp::r_sydney },
        { "us-central", dpp::r_us_central },
        { "us-east", dpp::r_us_east },
        { "us-south", dpp::r_us_south },
        { "us-west", dpp::r_us_west },
        { "western-europe", dpp::r_western_europe }
};

std::map<std::string, std::function<void(DiscordClient* client, json &j)>> events = {
	{
		"READY",
		[](DiscordClient* client, json &j) {
			client->logger->info("Shard {}/{} ready!", client->shard_id, client->max_shards);
			client->sessionid = j["d"]["session_id"];
		}
	},
	{
		"RESUMED",
		[](DiscordClient* client, json &j) {
			client->logger->debug("Successfully resumed session id {}", client->sessionid);
		}
	},
	{
		"GUILD_CREATE",
		[](DiscordClient* client, json &j) {
			dpp::guild* g = new dpp::guild();
			json& d = j["d"];

			g->id = from_string<uint64_t>(d["id"].get<std::string>(), std::dec);
			if (d.find("unavailable") == d.end() || d["unavailable"].get<bool>() == false) {
				g->name = d["name"].get<std::string>();
				g->icon = !d["icon"].is_null() ? d["icon"].get<std::string>() : "";
				g->splash = !d["splash"].is_null() ? d["splash"].get<std::string>() : "";
				g->discovery_splash = !d["discovery_splash"].is_null() ? d["discovery_splash"].get<std::string>() : "";
				g->owner_id = from_string<uint64_t>(d["owner_id"].get<std::string>(), std::dec);
				if (!d["region"].is_null()) {
					auto r = regionmap.find(d["region"].get<std::string>());
					if (r != regionmap.end()) {
						g->voice_region = r->second;
					}
				}

				g->flags |= (d.find("large") != d.end() && d["large"].get<bool>() == true) ? dpp::g_large : 0;
				g->flags |= (d.find("widget_enabled") != d.end() && d["widget_enabled"].get<bool>() == true) ? dpp::g_widget_enabled : 0;

				for (auto & feature : d["features"]) {
					auto f = featuremap.find(feature.get<std::string>());
					if (f != featuremap.end()) {
						g->flags |= f->second;
					}
				}
				uint8_t scf = d["system_channel_flags"].get<uint8_t>();
				if (scf & 1) {
					g->flags |= dpp::g_no_join_notifications;
				}
				if (scf & 2) {
					g->flags |= dpp::g_no_boost_notifications;
				}

				g->afk_channel_id = !d["afk_channel_id"].is_null() ? from_string<uint64_t>(d["afk_channel_id"].get<std::string>(), std::dec) : 0;
				g->afk_timeout = !d["afk_timeout"].is_null() ? d["afk_timeout"].get<uint32_t>() : 0;
				g->widget_channel_id = !d["widget_channel_id"].is_null() ? from_string<uint64_t>(d["widget_channel_id"].get<std::string>(), std::dec) : 0;
				g->verification_level = !d["verification_level"].is_null() ? d["verification_level"].get<uint8_t>() : 0;
				g->default_message_notifications = !d["default_message_notifications"].is_null() ? d["default_message_notifications"].get<uint8_t>() : 0;
				g->explicit_content_filter = !d["explicit_content_filter"].is_null() ? d["explicit_content_filter"].get<uint8_t>() : 0;
				g->mfa_level = !d["mfa_level"].is_null() ? d["mfa_level"].get<uint8_t>() : 0;
				g->application_id = !d["application_id"].is_null() ? from_string<uint64_t>(d["application_id"].get<std::string>(), std::dec) : 0;
				g->system_channel_id = !d["system_channel_id"].is_null() ? from_string<uint64_t>(d["system_channel_id"].get<std::string>(), std::dec) : 0;
				g->rules_channel_id = !d["rules_channel_id"].is_null() ? from_string<uint64_t>(d["rules_channel_id"].get<std::string>(), std::dec) : 0;
				//g->joined_at = 
				g->member_count = !d["member_count"].is_null() ? d["member_count"].get<uint32_t>() : 0;
				g->vanity_url_code = !d["vanity_url_code"].is_null() ? d["vanity_url_code"].get<std::string>() : "";
				g->description = !d["description"].is_null() ? d["description"].get<std::string>() : "";
				g->banner = !d["banner"].is_null() ? d["banner"].get<std::string>() : "";
				g->premium_tier = !d["premium_tier"].is_null() ? d["premium_tier"].get<uint8_t>() : 0;
				g->premium_subscription_count = !d["premium_subscription_count"].is_null() ? d["premium_subscription_count"].get<uint16_t>() : 0;
				g->public_updates_channel_id = !d["public_updates_channel_id"].is_null() ? from_string<int64_t>(d["public_updates_channel_id"].get<std::string>(), std::dec) : 0;
				g->max_video_channel_users = !d["max_video_channel_users"].is_null() ? d["max_video_channel_users"].get<uint32_t>() : 0;
			} else {
				g->flags |= dpp::g_unavailable;
			}
			
			dpp::store_guild(g);
		}
	},
	{
		"MESSAGE_UPDATE",
		[](DiscordClient* client, json &j) {
			 client->logger->debug("MESSAGE_UPDATE");
		}
	},
	{
		"MESSAGE_DELETE",
		[](DiscordClient* client, json &j) {
			 client->logger->debug("MESSAGE_DELETE");
		}
	},
	{
		"MESSAGE_CREATE",
		[](DiscordClient* client, json &j) {
			 client->logger->debug("MESSAGE_CREATE");
		}
	},
	{
		"CHANNEL_UPDATE",
		[](DiscordClient* client, json &j) {
			 client->logger->debug("CHANNEL_UPDATE");
		}
	},
	{
		"GUILD_ROLE_UPDATE",
		[](DiscordClient* client, json &j) {
			 client->logger->debug("GUILD_ROLE_UPDATE");
		}
	},
	{
		"MESSAGE_DELETE_BULK",
		[](DiscordClient* client, json &j) {
			 client->logger->debug("MESSAGE_DELETE_BULK");
		}
	},
	{
		"CHANNEL_CREATE",
		[](DiscordClient* client, json &j) {
			 client->logger->debug("CHANNEL_CREATE");
		}
	},
	{
		"CHANNEL_DELETE",
		[](DiscordClient* client, json &j) {
			 client->logger->debug("CHANNEL_DELETE");
		}
	},

};

void DiscordClient::HandleEvent(const std::string &event, json &j)
{
	auto ev_iter = events.find(event);
	if (ev_iter != events.end()) {
		ev_iter->second(this, j);
	} else {
		logger->debug("Unhamdled event: {}", event);
	}
}
