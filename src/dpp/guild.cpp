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
#include <dpp/discord.h>
#include <dpp/discordclient.h>
#include <dpp/voicestate.h>
#include <dpp/cache.h>
#include <dpp/discordevents.h>
#include <dpp/stringops.h>
#include <dpp/nlohmann/json.hpp>
#include <dpp/fmt/format.h>

using json = nlohmann::json;

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


namespace dpp {

guild::guild() :
	managed(),
	shard_id(0),
	flags(0),
	owner_id(0),
	afk_channel_id(0),
	afk_timeout(0),
	widget_channel_id(0),
	verification_level(ver_none),
	default_message_notifications(0),
	explicit_content_filter(expl_disabled),
	mfa_level(mfa_none),
	application_id(0),
	system_channel_id(0),
	rules_channel_id(0),
	member_count(0),
	premium_tier(0),
	premium_subscription_count(0),
	public_updates_channel_id(0),
	max_video_channel_users(0),
	max_presences(0),
	max_members(0),
	nsfw_level(nsfw_default)
{
}


guild_member::guild_member() :
	joined_at(0),
	premium_since(0),
	flags(0)
{
}

guild_member& guild_member::fill_from_json(nlohmann::json* j, snowflake g_id, snowflake u_id) {
	this->guild_id = g_id;
	this->user_id = u_id;
	j->get_to(*this);
	return *this;
}

void from_json(const nlohmann::json& j, guild_member& gm) {
	gm.nickname = StringNotNull(&j, "nick");
	gm.joined_at = TimestampNotNull(&j, "joined_at");
	gm.premium_since = TimestampNotNull(&j, "premium_since");

	gm.roles.clear();
	if (j.contains("roles") && !j.at("roles").is_null()) {
		gm.roles.reserve(j.at("roles").size());
		for (auto& role : j.at("roles")) {
			gm.roles.push_back(std::stoull(role.get<std::string>()));
		}
	}

	if (j.contains("avatar") && !j.at("avatar").is_null()) {
		std::string av = StringNotNull(&j, "avatar");
		if (av.substr(0, 2) == "a_") {
			gm.flags |= gm_animated_avatar;
		}
		gm.avatar = av;
	}

	gm.flags |= BoolNotNull(&j, "deaf") ? gm_deaf : 0;
	gm.flags |= BoolNotNull(&j, "mute") ? gm_mute : 0;
	gm.flags |= BoolNotNull(&j, "pending") ? gm_pending : 0;
}

std::string guild_member::get_avatar_url()  const {
	/* XXX: Discord were supposed to change their CDN over to discord.com, they haven't.
	 * At some point in the future this URL *will* change!
	 */
	if (!this->avatar.to_string().empty()) {
		return fmt::format("https://cdn.discordapp.com/avatars/{}/{}{}.{}",
			this->user_id,
			(has_animated_guild_avatar() ? "a_" : ""),
			this->avatar.to_string(),
			(has_animated_guild_avatar() ? "gif" : "png")
		);
	} else {
		return std::string();
	}
}


bool guild_member::has_animated_guild_avatar() const {
	return this->flags & gm_animated_avatar;
}

std::string guild_member::build_json() const {
	json j;
	if (!this->nickname.empty())
		j["nick"] = this->nickname;
	if (this->roles.size()) {
		j["roles"] = {};
		for (auto & role : roles) {
			j["roles"].push_back(std::to_string(role));
		}
	}
	if (is_muted()) {
		j["muted"] = true;
	}
	if (is_deaf()) {
		j["deaf"] = true;
	}
	return j.dump();
}

bool guild_member::is_deaf() const {
	return flags & dpp::gm_deaf;
}

bool guild_member::is_muted() const {
	return flags & dpp::gm_mute;
}

bool guild_member::is_pending() const {
	return flags & dpp::gm_pending;
}

bool guild::is_large() const {
	return this->flags & g_large;
}

bool guild::is_unavailable() const {
	return this->flags & g_unavailable;
}

bool guild::widget_enabled() const {
	return this->flags & g_widget_enabled;
}

bool guild::has_invite_splash() const {
	return this->flags & g_invite_splash;
}

bool guild::has_vip_regions() const {
	return this->flags & g_vip_regions;
}

bool guild::has_vanity_url() const {
	return this->flags & g_vanity_url;
}

bool guild::is_verified() const {
	return this->flags & g_verified;
}

bool guild::is_partnered() const {
	return this->flags & g_partnered;
}

bool guild::is_community() const {
	return this->flags & g_community;
}

bool guild::has_commerce() const {
	return this->flags & g_commerce;
}

bool guild::has_news() const {
	return this->flags & g_news;
}

bool guild::is_discoverable() const {
	return this->flags & g_discoverable;
}

bool guild::is_featureable() const {
	return this->flags & g_featureable;
}

bool guild::has_animated_icon() const {
	return this->flags & g_animated_icon;
}

bool guild::has_banner() const {
	return this->flags & g_banner;
}

bool guild::is_welcome_screen_enabled() const {
	return this->flags & g_welcome_screen_enabled;
}

bool guild::has_member_verification_gate() const {
	return this->flags & g_member_verification_gate;
}

bool guild::is_preview_enabled() const {
	return this->flags & g_preview_enabled;
}

bool guild::has_animated_icon_hash() const {
	return this->flags & g_has_animated_icon;
}

bool guild::has_animated_banner_icon_hash() const {
	return this->flags & g_has_animated_banner;
}

std::string guild::build_json(bool with_id) const {
	json j;
	if (with_id) {
		j["id"] = std::to_string(id);
	}
	if (!name.empty()) {
		j["name"] = name;
	}
	j["widget_enabled"] = widget_enabled();
	if (afk_channel_id) {
		j["afk_channel_id"] = afk_channel_id;
	}
	if (afk_channel_id) {
		j["afk_timeout"] = afk_timeout;
	}
	if (widget_enabled()) {
		j["widget_channel_id"] = widget_channel_id;
	}
	j["default_message_notifications"] = default_message_notifications;
	j["explicit_content_filter"] = explicit_content_filter;
	j["mfa_level"] = mfa_level;
	if (system_channel_id) {
		j["system_channel_id"] = system_channel_id;
	}
	if (rules_channel_id) {
		j["rules_channel_id"] = rules_channel_id;
	}
	if (!vanity_url_code.empty()) {
		j["vanity_url_code"] = vanity_url_code;
	}
	if (!description.empty()) {
		j["description"] = description;
	}
	return j.dump();
}

void guild::rehash_members() {
	members_container n;
	n.reserve(members.size());
	for (auto t = members.begin(); t != members.end(); ++t) {
		n.insert(*t);
	}
	members = n;
}


guild& guild::fill_from_json(discord_client* shard, nlohmann::json* d) {
	/* NOTE: This can be called by GUILD_UPDATE and also GUILD_CREATE.
	 * GUILD_UPDATE sends a partial guild object, so we use Set*NotNull functions
	 * for a lot of the values under the assumption they may sometimes be missing.
	 */
	this->id = SnowflakeNotNull(d, "id");
	if (d->find("unavailable") == d->end() || (*d)["unavailable"].get<bool>() == false) {
		/* Clear unavailable flag if set */
		if (this->flags & dpp::g_unavailable) {
			this->flags -= dpp::g_unavailable;
		}
		SetStringNotNull(d, "name", this->name);
		/* Special case for guild icon to allow for animated icons.
		 * Animated icons start with a_ on the name, so we use this to set a flag
		 * in the flags field and then just store the iconhash separately.
		 */
		std::string _icon = StringNotNull(d, "icon");
		if (!_icon.empty()) {
			if (_icon.length() > 2 && _icon.substr(0, 2) == "a_") {
				_icon = _icon.substr(2, _icon.length());
				this->flags |= g_has_animated_icon;
			}
			this->icon = _icon;
		}
		std::string _dsplash = StringNotNull(d, "discovery_splash");
		if (!_dsplash.empty()) {
			this->discovery_splash = _dsplash;
		}
		SetSnowflakeNotNull(d, "owner_id", this->owner_id);

		this->flags |= BoolNotNull(d, "large") ? dpp::g_large : 0;
		this->flags |= BoolNotNull(d, "widget_enabled") ? dpp::g_widget_enabled : 0;

		for (auto & feature : (*d)["features"]) {
			auto f = featuremap.find(feature.get<std::string>());
			if (f != featuremap.end()) {
				this->flags |= f->second;
			}
		}
		uint8_t scf = Int8NotNull(d, "system_channel_flags");
		if (scf & 1) {
			this->flags |= dpp::g_no_join_notifications;
		}
		if (scf & 2) {
			this->flags |= dpp::g_no_boost_notifications;
		}
		if (scf & 4) {
			this->flags |= dpp::g_no_setup_tips;
		}
		if (scf & 8) {
a			this->flags |= dpp::g_no_sticker_greeting;
		}

		SetSnowflakeNotNull(d, "afk_channel_id", this->afk_channel_id);
		SetInt8NotNull(d, "afk_timeout", this->afk_timeout);
		SetSnowflakeNotNull(d, "widget_channel_id", this->widget_channel_id);
		this->verification_level = (verification_level_t)Int8NotNull(d, "verification_level");
		SetInt8NotNull(d, "default_message_notifications", this->default_message_notifications);
		this->explicit_content_filter = (guild_explicit_content_t)Int8NotNull(d, "explicit_content_filter");
		this->mfa_level = (mfa_level_t)Int8NotNull(d, "mfa_level");
		SetSnowflakeNotNull(d, "application_id", this->application_id);
		SetSnowflakeNotNull(d, "system_channel_id", this->system_channel_id);
		SetSnowflakeNotNull(d, "rules_channel_id", this->rules_channel_id);
		SetInt32NotNull(d, "member_count", this->member_count);
		SetStringNotNull(d, "vanity_url_code", this->vanity_url_code);
		SetStringNotNull(d, "description", this->description);
		if (d->find("voice_states") != d->end()) {
			this->voice_members.clear();
			for (auto & vm : (*d)["voice_states"]) {
				voicestate vs;
				vs.fill_from_json(&vm);
				vs.shard = shard;
				vs.guild_id = this->id;
				this->voice_members[vs.user_id] = vs;
			}
		}

		std::string _banner = StringNotNull(d, "banner");
		if (!_banner.empty()) {
			if (_banner.length() > 2 && _banner.substr(0, 2) == "a_") {
				this->flags |= dpp::g_has_animated_banner;
			}
			this->banner = _banner;
		}
		SetInt8NotNull(d, "premium_tier", this->premium_tier);
		SetInt16NotNull(d, "premium_subscription_count", this->premium_subscription_count);
		SetSnowflakeNotNull(d, "public_updates_channel_id", this->public_updates_channel_id);
		SetInt16NotNull(d, "max_video_channel_users", this->max_video_channel_users);

		SetInt32NotNull(d, "max_presences", this->max_presences);
		SetInt32NotNull(d, "max_members", this->max_members);

		this->nsfw_level = (guild_nsfw_level_t)Int8NotNull(d, "nsfw_level");

		if (d->find("welcome_screen") != d->end()) {
			json& w = (*d)["welcome_screen"];
			SetStringNotNull(&w, "description", welcome_screen.description);
			welcome_screen.welcome_channels.reserve(w["welcome_channels"].size());
			for (auto& wc : w["welcome_channels"]) {
				welcome_channel_t wchan;
				SetStringNotNull(&wc, "description", wchan.description);
				SetSnowflakeNotNull(&wc, "channel_id", wchan.channel_id);
				SetSnowflakeNotNull(&wc, "emoji_id", wchan.emoji_id);
				SetStringNotNull(&wc, "emoji_name", wchan.emoji_name);
				welcome_screen.welcome_channels.emplace_back(wchan);
			}
		}
		
	} else {
		this->flags |= dpp::g_unavailable;
	}
	return *this;
}

guild_widget::guild_widget() : enabled(false), channel_id(0)
{
}

guild_widget& guild_widget::fill_from_json(nlohmann::json* j) {
	enabled = BoolNotNull(j, "enabled");
	channel_id = SnowflakeNotNull(j, "channel_id");
	return *this;
}

std::string guild_widget::build_json() const {
	return json({{"channel_id", channel_id}, {"enabled", enabled}}).dump();
}


uint64_t guild::base_permissions(const user* member) const
{
	if (owner_id == member->id)
		return ~0;

	role* everyone = dpp::find_role(id);
	auto mi = members.find(member->id);
	if (mi == members.end())
		return 0;
	guild_member gm = mi->second;

	uint64_t permissions = everyone->permissions;

	for (auto& rid : gm.roles) {
		role* r = dpp::find_role(rid);
		permissions |= r->permissions;
	}

	if (permissions & p_administrator)
		return ~0;

	return permissions;
}

uint64_t guild::permission_overwrites(const uint64_t base_permissions, const user*  member, const channel* channel) const
{
	if (base_permissions & p_administrator)
		return ~0;

	int64_t permissions = base_permissions;
	for (auto it = channel->permission_overwrites.begin(); it != channel->permission_overwrites.end(); ++it) {
		if (it->id == id && it->type == ot_role) {
			permissions &= ~it->deny;
			permissions |= it->allow;
			break;
		}
	}

	auto mi = members.find(member->id);
	if (mi == members.end())
		return 0;
	guild_member gm = mi->second;
	uint64_t allow = 0;
	uint64_t deny = 0;

	for (auto& rid : gm.roles) {

		/* Skip \@everyone, calculated above */
		if (rid == id)
			continue;

		for (auto it = channel->permission_overwrites.begin(); it != channel->permission_overwrites.end(); ++it) {
			if ((rid == it->id && it->type == ot_role) || (member->id == it->id && it->type == ot_member)) {
				allow |= it->allow;
				deny |= it->deny;
				break;
			}
		}
	}

	permissions &= ~deny;
	permissions |= allow;

	return permissions;
}

bool guild::connect_member_voice(snowflake user_id, bool self_mute, bool self_deaf) {
	for (auto & c : channels) {
		channel* ch = dpp::find_channel(c);
		if (!ch || (!ch->is_voice_channel() && !ch->is_stage_channel())) {
			continue;
		}
		auto vcmembers = ch->get_voice_members();
		auto vsi = vcmembers.find(user_id);
		if (vsi != vcmembers.end()) {
			if (vsi->second.shard) {
				vsi->second.shard->connect_voice(this->id, vsi->second.channel_id, self_mute, self_deaf);
				return true;
			}
		}
	}
	return false;
}



};
