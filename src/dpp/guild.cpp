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
#include <dpp/cache.h>
#include <dpp/discordclient.h>
#include <dpp/voicestate.h>
#include <dpp/exception.h>
#include <dpp/guild.h>
#include <dpp/discordevents.h>
#include <dpp/stringops.h>
#include <dpp/nlohmann/json.hpp>

using json = nlohmann::json;

const std::map<std::string, std::variant<dpp::guild_flags, dpp::guild_flags_extra>> featuremap = {
	{"INVITE_SPLASH", dpp::g_invite_splash },
	{"VIP_REGIONS", dpp::g_vip_regions },
	{"VANITY_URL", dpp::g_vanity_url },
	{"VERIFIED", dpp::g_verified },
	{"PARTNERED", dpp::g_partnered },
	{"COMMUNITY", dpp::g_community },
	{"DEVELOPER_SUPPORT_SERVER", dpp::g_developer_support_server },
	{"COMMERCE", dpp::g_commerce },
	{"NEWS", dpp::g_news },
	{"DISCOVERABLE", dpp::g_discoverable },
	{"FEATURABLE", dpp::g_featureable },
	{"INVITES_DISABLED", dpp::g_invites_disabled},
	{"ANIMATED_BANNER", dpp::g_animated_banner },
	{"ANIMATED_ICON", dpp::g_animated_icon },
	{"BANNER", dpp::g_banner },
	{"WELCOME_SCREEN_ENABLED", dpp::g_welcome_screen_enabled },
	{"MEMBER_VERIFICATION_GATE_ENABLED", dpp::g_member_verification_gate },
	{"PREVIEW_ENABLED", dpp::g_preview_enabled },
	{"MONETIZATION_ENABLED", dpp::g_monetization_enabled },
	{"MORE_STICKERS", dpp::g_more_stickers },
	{"ROLE_ICONS", dpp::g_role_icons },
	{"SEVEN_DAY_THREAD_ARCHIVE", dpp::g_seven_day_thread_archive },
	{"THREE_DAY_THREAD_ARCHIVE", dpp::g_three_day_thread_archive },
	{"TICKETED_EVENTS_ENABLED", dpp::g_ticketed_events },
	{"CHANNEL_BANNER", dpp::g_channel_banners },
	{"AUTO_MODERATION", dpp::g_auto_moderation },
};

namespace dpp {

guild::guild() :
	managed(),
	owner_id(0),
	afk_channel_id(0),
	application_id(0),
	system_channel_id(0),
	rules_channel_id(0),
	public_updates_channel_id(0),
	widget_channel_id(0),
	member_count(0),
	flags(0),
	max_presences(0),
	max_members(0),
	shard_id(0),
	premium_subscription_count(0),
	afk_timeout(afk_off),
	max_video_channel_users(0),
	default_message_notifications(dmn_all),
	premium_tier(tier_none),
	verification_level(ver_none),
	explicit_content_filter(expl_disabled),
	mfa_level(mfa_none),
	nsfw_level(nsfw_default),
	flags_extra(0)
{
}


guild_member::guild_member() :
	guild_id(0),
	user_id(0),
	communication_disabled_until(0),
	joined_at(0),
	premium_since(0),
	flags(0)
{
}

std::string guild_member::get_mention() const {
	return "<@" + std::to_string(user_id) + ">";
}

guild_member& guild_member::set_nickname(const std::string& nick) {
	this->nickname = nick;
	return *this;
}

guild_member& guild_member::set_mute(const bool is_muted) {
	this->flags = (is_muted) ? flags | gm_mute : flags & ~gm_mute;
	this->flags |= gm_voice_action;
	return *this;
}

guild_member& guild_member::set_deaf(const bool is_deafened) {
	this->flags = (is_deafened) ? flags | gm_deaf : flags & ~gm_deaf;
	this->flags |= gm_voice_action;
	return *this;
}

guild_member& guild_member::set_communication_disabled_until(const time_t disabled_timestamp) {
	this->communication_disabled_until = disabled_timestamp;
	return *this;
}

guild_member& guild_member::fill_from_json(nlohmann::json* j, snowflake g_id, snowflake u_id) {
	this->guild_id = g_id;
	this->user_id = u_id;
	j->get_to(*this);

	return *this;
}

bool guild_member::is_communication_disabled() const {
	return communication_disabled_until > time(nullptr);
}

void from_json(const nlohmann::json& j, guild_member& gm) {
	set_string_not_null(&j, "nick", gm.nickname);
	set_ts_not_null(&j, "joined_at", gm.joined_at);
	set_ts_not_null(&j, "premium_since", gm.premium_since);
	set_ts_not_null(&j, "communication_disabled_until", gm.communication_disabled_until);

	gm.roles.clear();
	if (j.contains("roles") && !j.at("roles").is_null()) {
		gm.roles.reserve(j.at("roles").size());
		for (auto& role : j.at("roles")) {
			gm.roles.push_back(std::stoull(role.get<std::string>()));
		}
	}

	if (j.contains("avatar") && !j.at("avatar").is_null()) {
		std::string av = string_not_null(&j, "avatar");
		if (av.substr(0, 2) == "a_") {
			gm.flags |= gm_animated_avatar;
		}
		gm.avatar = av;
	}

	gm.flags |= bool_not_null(&j, "deaf") ? gm_deaf : 0;
	gm.flags |= bool_not_null(&j, "mute") ? gm_mute : 0;
	gm.flags |= bool_not_null(&j, "pending") ? gm_pending : 0;
}

std::string guild_member::get_avatar_url(uint16_t size)  const {
	/* XXX: Discord were supposed to change their CDN over to discord.com, they haven't.
	 * At some point in the future this URL *will* change!
	 */
	if (!this->avatar.to_string().empty()) {

		return utility::cdn_host + "/guilds/" +
			std::to_string(this->guild_id) + 
			"/" +
			std::to_string(this->user_id) +
			(has_animated_guild_avatar() ? "/a_" : "/") +
			this->avatar.to_string() +
			(has_animated_guild_avatar() ? ".gif" : ".png") +
			utility::avatar_size(size);
	} else {
		return std::string();
	}
}


bool guild_member::has_animated_guild_avatar() const {
	return this->flags & gm_animated_avatar;
}

std::string guild_member::build_json([[maybe_unused]] bool with_id) const {
	json j;
	if (this->communication_disabled_until > 0) {
		if (this->communication_disabled_until > std::time(nullptr)) {
			j["communication_disabled_until"] = ts_to_string(this->communication_disabled_until);
		} else {
			j["communication_disabled_until"] = json::value_t::null;
		}
	}
	if (!this->nickname.empty())
		j["nick"] = this->nickname;
	if (!this->roles.empty()) {
		j["roles"] = {};
		for (auto & role : roles) {
			j["roles"].push_back(std::to_string(role));
		}
	}

	if (flags & gm_voice_action) {
		j["mute"] = is_muted();
		j["deaf"] = is_deaf();
	}

	return j.dump();
}

guild& guild::set_name(const std::string& n) {
	this->name = utility::validate(trim(n), 2, 100, "Guild names cannot be less than 2 characters");
	return *this;
}

dpp::user* guild_member::get_user() const {
	return dpp::find_user(user_id);
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

bool guild::has_premium_progress_bar_enabled() const {
	return this->flags_extra & g_premium_progress_bar_enabled;
}

bool guild::has_invites_disabled() const {
	return this->flags_extra & g_invites_disabled;
}

bool guild::has_channel_banners() const {
	return this->flags & g_channel_banners;
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

bool guild::has_animated_banner() const {
	return this->flags_extra & g_animated_banner;
}

bool guild::has_auto_moderation() const {
	return this->flags_extra & g_auto_moderation;
}

bool guild::has_support_server() const {
	return this->flags_extra & g_developer_support_server;
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

bool guild::has_animated_banner_hash() const {
	return this->flags & g_has_animated_banner;
}

bool guild::has_monetization_enabled() const {
	return this->flags & g_monetization_enabled;
}

bool guild::has_more_stickers() const {
	return this->flags & g_more_stickers;
}

bool guild::has_private_threads() const {
	return this->flags & g_private_threads;
}

bool guild::has_role_icons() const {
	return this->flags & g_role_icons;
}

bool guild::has_seven_day_thread_archive() const {
	return this->flags & g_seven_day_thread_archive;
}

bool guild::has_three_day_thread_archive() const {
	return this->flags & g_three_day_thread_archive;
}

bool guild::has_ticketed_events() const {
	return this->flags & g_ticketed_events;
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
	if (afk_timeout) {
		if (afk_timeout == afk_60) {
			j["afk_timeout"] = 60;
		} else if (afk_timeout == afk_300) {
			j["afk_timeout"] = 300;
		} else if (afk_timeout == afk_900) {
			j["afk_timeout"] = 900;
		} else if (afk_timeout == afk_1800) {
			j["afk_timeout"] = 1800;
		} else if (afk_timeout == afk_3600) {
			j["afk_timeout"] = 3600;
		}
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
	j["premium_progress_bar_enabled"] = (bool)(flags_extra & g_premium_progress_bar_enabled);
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

guild& guild::fill_from_json(nlohmann::json* d) {
	return fill_from_json(nullptr, d);
}


guild& guild::fill_from_json(discord_client* shard, nlohmann::json* d) {
	/* NOTE: This can be called by GUILD_UPDATE and also GUILD_CREATE.
	 * GUILD_UPDATE sends a partial guild object, so we use Set*NotNull functions
	 * for a lot of the values under the assumption they may sometimes be missing.
	 */
	this->id = snowflake_not_null(d, "id");
	if (d->find("unavailable") == d->end() || (*d)["unavailable"].get<bool>() == false) {
		/* Clear unavailable flag if set */
		if (this->flags & dpp::g_unavailable) {
			this->flags -= dpp::g_unavailable;
		}
		set_string_not_null(d, "name", this->name);
		/* Special case for guild icon to allow for animated icons.
		 * Animated icons start with a_ on the name, so we use this to set a flag
		 * in the flags field and then just store the iconhash separately.
		 */
		std::string _icon = string_not_null(d, "icon");
		if (!_icon.empty()) {
			if (_icon.length() > 2 && _icon.substr(0, 2) == "a_") {
				_icon = _icon.substr(2, _icon.length());
				this->flags |= g_has_animated_icon;
			}
			this->icon = _icon;
		}
		std::string _dsplash = string_not_null(d, "discovery_splash");
		if (!_dsplash.empty()) {
			this->discovery_splash = _dsplash;
		}
		set_snowflake_not_null(d, "owner_id", this->owner_id);

		this->flags |= bool_not_null(d, "large") ? dpp::g_large : 0;
		this->flags |= bool_not_null(d, "widget_enabled") ? dpp::g_widget_enabled : 0;

		this->flags_extra |= bool_not_null(d, "premium_progress_bar_enabled") ? dpp::g_premium_progress_bar_enabled : 0;

		for (auto & feature : (*d)["features"]) {
			auto f = featuremap.find(feature.get<std::string>());
			if (f != featuremap.end()) {
				if (std::holds_alternative<guild_flags_extra>(f->second)) {
					this->flags_extra |= std::get<guild_flags_extra>(f->second);
				} else {
					this->flags |= std::get<guild_flags>(f->second);
				}
			}
		}
		uint8_t scf = int8_not_null(d, "system_channel_flags");
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
			this->flags |= dpp::g_no_sticker_greeting;
		}

		if (d->contains("afk_timeout")) {
			if ((*d)["afk_timeout"] == 60) {
				this->afk_timeout = afk_60;
			} else if ((*d)["afk_timeout"] == 300) {
				this->afk_timeout = afk_300;
			} else if ((*d)["afk_timeout"] == 900) {
				this->afk_timeout = afk_900;
			} else if ((*d)["afk_timeout"] == 1800) {
				this->afk_timeout = afk_1800;
			} else if ((*d)["afk_timeout"] == 3600) {
				this->afk_timeout = afk_3600;
			}
		}
		set_snowflake_not_null(d, "afk_channel_id", this->afk_channel_id);
		set_snowflake_not_null(d, "widget_channel_id", this->widget_channel_id);
		this->verification_level = (verification_level_t)int8_not_null(d, "verification_level");
		this->default_message_notifications = (default_message_notification_t)int8_not_null(d, "default_message_notifications");
		this->explicit_content_filter = (guild_explicit_content_t)int8_not_null(d, "explicit_content_filter");
		this->mfa_level = (mfa_level_t)int8_not_null(d, "mfa_level");
		set_snowflake_not_null(d, "application_id", this->application_id);
		set_snowflake_not_null(d, "system_channel_id", this->system_channel_id);
		set_snowflake_not_null(d, "rules_channel_id", this->rules_channel_id);
		set_int32_not_null(d, "member_count", this->member_count);
		set_string_not_null(d, "vanity_url_code", this->vanity_url_code);
		set_string_not_null(d, "description", this->description);
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

		std::string _banner = string_not_null(d, "banner");
		if (!_banner.empty()) {
			if (_banner.length() > 2 && _banner.substr(0, 2) == "a_") {
				this->flags |= dpp::g_has_animated_banner;
			}
			this->banner = _banner;
		}
		this->premium_tier = (guild_premium_tier_t)int8_not_null(d, "premium_tier");
		set_int16_not_null(d, "premium_subscription_count", this->premium_subscription_count);
		set_snowflake_not_null(d, "public_updates_channel_id", this->public_updates_channel_id);
		set_int8_not_null(d, "max_video_channel_users", this->max_video_channel_users);

		set_int32_not_null(d, "max_presences", this->max_presences);
		set_int32_not_null(d, "max_members", this->max_members);

		this->nsfw_level = (guild_nsfw_level_t)int8_not_null(d, "nsfw_level");

		if (d->find("welcome_screen") != d->end()) {
			json& w = (*d)["welcome_screen"];
			set_string_not_null(&w, "description", welcome_screen.description);
			welcome_screen.welcome_channels.reserve(w["welcome_channels"].size());
			for (auto& wc : w["welcome_channels"]) {
				welcome_channel_t wchan;
				set_string_not_null(&wc, "description", wchan.description);
				set_snowflake_not_null(&wc, "channel_id", wchan.channel_id);
				set_snowflake_not_null(&wc, "emoji_id", wchan.emoji_id);
				set_string_not_null(&wc, "emoji_name", wchan.emoji_name);
				welcome_screen.welcome_channels.emplace_back(wchan);
			}
		}
		
	} else {
		this->flags |= dpp::g_unavailable;
	}
	return *this;
}

guild_widget::guild_widget() : channel_id(0), enabled(false)
{
}

guild_widget& guild_widget::fill_from_json(nlohmann::json* j) {
	enabled = bool_not_null(j, "enabled");
	channel_id = snowflake_not_null(j, "channel_id");
	return *this;
}

std::string guild_widget::build_json([[maybe_unused]] bool with_id) const {
	return json({{"channel_id", channel_id}, {"enabled", enabled}}).dump();
}


permission guild::base_permissions(const user* user) const {
	if (user == nullptr)
		return 0;

	auto mi = members.find(user->id);
	if (mi == members.end())
		return 0;
	guild_member gm = mi->second;

	return base_permissions(gm);
}

permission guild::base_permissions(const guild_member &member) const {

	/* this method is written with the help of discord's pseudocode available here https://discord.com/developers/docs/topics/permissions#permission-overwrites */

	if (owner_id == member.user_id)
		return ~0; // return all permissions if it's the owner of the guild

	role* everyone = dpp::find_role(id);
	if (everyone == nullptr)
		return 0;

	permission permissions = everyone->permissions;

	for (auto& rid : member.roles) {
		role* r = dpp::find_role(rid);
		if (r) {
			permissions |= r->permissions;
		}
	}

	if (permissions & p_administrator)
		return ~0;

	return permissions;
}

permission guild::permission_overwrites(const uint64_t base_permissions, const user* user, const channel* channel) const {
	if (user == nullptr || channel == nullptr)
		return 0;

	/* this method is written with the help of discord's pseudocode available here https://discord.com/developers/docs/topics/permissions#permission-overwrites */

	// ADMINISTRATOR overrides any potential permission overwrites, so there is nothing to do here.
	if (base_permissions & p_administrator)
		return ~0;

	permission permissions = base_permissions;

	// find \@everyone role overwrite and apply it.
	for (auto it = channel->permission_overwrites.begin(); it != channel->permission_overwrites.end(); ++it) {
		if (it->id == this->id && it->type == ot_role) {
			permissions &= ~it->deny;
			permissions |= it->allow;
			break;
		}
	}

	auto mi = members.find(user->id);
	if (mi == members.end())
		return 0;
	guild_member gm = mi->second;

	// Apply role specific overwrites.
	uint64_t allow = 0;
	uint64_t deny = 0;

	for (auto& rid : gm.roles) {

		/* Skip \@everyone role to not break the hierarchy. It's calculated above */
		if (rid == this->id)
			continue;

		for (auto it = channel->permission_overwrites.begin(); it != channel->permission_overwrites.end(); ++it) {
			if (rid == it->id && it->type == ot_role) {
				deny |= it->deny;
				allow |= it->allow;
				break;
			}
		}
	}

	permissions &= ~deny;
	permissions |= allow;

	// Apply member specific overwrite if exists.
	for (auto it = channel->permission_overwrites.begin(); it != channel->permission_overwrites.end(); ++it) {
		if (gm.user_id == it->id && it->type == ot_member) {
			permissions &= ~it->deny;
			permissions |= it->allow;
			break;
		}
	}

	return permissions;
}

permission guild::permission_overwrites(const guild_member &member, const channel &channel) const {

	permission base_permissions = this->base_permissions(member);

	/* this method is written with the help of discord's pseudocode available here https://discord.com/developers/docs/topics/permissions#permission-overwrites */

	// ADMINISTRATOR overrides any potential permission overwrites, so there is nothing to do here.
	if (base_permissions & p_administrator)
		return ~0;

	permission permissions = base_permissions;

	// find \@everyone role overwrite and apply it.
	for (auto it = channel.permission_overwrites.begin(); it != channel.permission_overwrites.end(); ++it) {
		if (it->id == this->id && it->type == ot_role) {
			permissions &= ~it->deny;
			permissions |= it->allow;
			break;
		}
	}

	// Apply role specific overwrites.
	uint64_t allow = 0;
	uint64_t deny = 0;

	for (auto& rid : member.roles) {

		/* Skip \@everyone role to not break the hierarchy. It's calculated above */
		if (rid == this->id)
			continue;

		for (auto it = channel.permission_overwrites.begin(); it != channel.permission_overwrites.end(); ++it) {
			if (rid == it->id && it->type == ot_role) {
				deny |= it->deny;
				allow |= it->allow;
				break;
			}
		}
	}

	permissions &= ~deny;
	permissions |= allow;

	// Apply member specific overwrite if exists.
	for (auto it = channel.permission_overwrites.begin(); it != channel.permission_overwrites.end(); ++it) {
		if (member.user_id == it->id && it->type == ot_member) {
			permissions &= ~it->deny;
			permissions |= it->allow;
			break;
		}
	}

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

std::string guild::get_banner_url(uint16_t size) const {
	/* XXX: Discord were supposed to change their CDN over to discord.com, they haven't.
	 * At some point in the future this URL *will* change!
	 */
	if (!this->banner.to_string().empty()) {
		return utility::cdn_host + "/banners/" +
			std::to_string(this->id) +
			(has_animated_banner_hash() ? "/a_" : "/") +
			this->banner.to_string() +
			(has_animated_banner_hash() ? ".gif" : ".png") +
			utility::avatar_size(size);
	} else {
		return std::string();
	}
}

std::string guild::get_discovery_splash_url(uint16_t size) const {
	/* XXX: Discord were supposed to change their CDN over to discord.com, they haven't.
	 * At some point in the future this URL *will* change!
	 */
	if (!this->discovery_splash.to_string().empty()) {
		return utility::cdn_host + "/discovery-splashes/" +
			std::to_string(this->id) + "/" +
			this->discovery_splash.to_string() +
			".png" +
			utility::avatar_size(size);
	} else {
		return std::string();
	}
}

std::string guild::get_icon_url(uint16_t size) const {
	/* XXX: Discord were supposed to change their CDN over to discord.com, they haven't.
	 * At some point in the future this URL *will* change!
	 */
	if (!this->icon.to_string().empty()) {
		return utility::cdn_host + "/icons/" +
			std::to_string(this->id) +
			(has_animated_icon_hash() ? "/a_" : "/") +
			this->icon.to_string() +
			(has_animated_icon_hash() ? ".gif" : ".png") +
			utility::avatar_size(size);
	} else {
		return std::string();
	}
}

std::string guild::get_splash_url(uint16_t size) const {
	/* XXX: Discord were supposed to change their CDN over to discord.com, they haven't.
	 * At some point in the future this URL *will* change!
	 */
	if (!this->splash.to_string().empty()) {
		return utility::cdn_host + "/splashes/" +
			std::to_string(this->id) + "/" + 
			this->splash.to_string() +
			utility::avatar_size(size);
	} else {
		return std::string();
	}
}

guild_member find_guild_member(const snowflake guild_id, const snowflake user_id) {
	guild* g = find_guild(guild_id);
	if (g) {
		auto gm = g->members.find(user_id);
		if (gm != g->members.end()) {
			return gm->second;
		}

		throw dpp::cache_exception("Requested member not found in the guild cache!");
	}
	
	throw dpp::cache_exception("Requested guild cache not found!");
}


};
