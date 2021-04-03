#pragma once

namespace dpp {

enum region {
	r_brazil,
	r_central_europe,
	r_hong_kong,
	r_india,
	r_japan,
	r_russia,
	r_singapore,
	r_south_africa,
	r_sydney,
	r_us_central,
	r_us_east,
	r_us_south,
	r_us_west,
	r_western_europe
};

enum guild_flags {
	g_large =				0b00000000000000000001,
	g_unavailable = 			0b00000000000000000010,
	g_widget_enabled =			0b00000000000000000100,
	g_invite_splash =			0b00000000000000001000,
	g_vip_regions =				0b00000000000000010000,
	g_vanity_url =				0b00000000000000100000,
	g_verified =				0b00000000000001000000,
	g_partnered =				0b00000000000010000000,
	g_community =				0b00000000000100000000,
	g_commerce =				0b00000000001000000000,
	g_news =				0b00000000010000000000,
	g_discoverable =			0b00000000100000000000,
	g_featureable =				0b00000001000000000000,
	g_animated_icon =			0b00000010000000000000,
	g_banner =				0b00000100000000000000,
	g_welcome_screen_enabled =		0b00001000000000000000,
	g_member_verification_gate =		0b00010000000000000000,
	g_preview_enabled =			0b00100000000000000000,
	g_no_join_notifications =		0b01000000000000000000,
	g_no_boost_notifications =		0b10000000000000000000
};

class guild : public managed {
public:
	uint32_t flags;
	std::string name;
	std::string icon;
	std::string splash;
	std::string discovery_splash;
	snowflake owner_id;
	region voice_region;
	snowflake afk_channel_id;
	uint32_t afk_timeout;
	snowflake widget_channel_id;
	uint8_t verification_level;
	uint8_t default_message_notifications;
	uint8_t explicit_content_filter;
	uint8_t mfa_level;
	snowflake application_id;
	snowflake system_channel_id;
	snowflake rules_channel_id;
	uint32_t member_count;
	std::string vanity_url_code;
	std::string description;
	std::string banner;
	uint8_t premium_tier;
	uint16_t premium_subscription_count;
	snowflake public_updates_channel_id;
	uint32_t max_video_channel_users;

	std::vector<snowflake> roles;
	std::vector<snowflake> channels;
	std::map<snowflake, class guild_member*> members;

	guild();
	~guild();
	void fill_from_json(nlohmann::json* j);
	std::string build_json(bool with_id = false) const;

	bool is_large() const;
	bool is_unavailable() const;
	bool widget_enabled() const;
	bool has_invite_splash() const;
	bool has_vip_regions() const;
	bool has_vanity_url() const;
	bool is_verified() const;
	bool is_partnered() const;
	bool is_community() const;
	bool has_commerce() const;
	bool has_news() const;
	bool is_discoverable() const;
	bool is_featureable() const;
	bool has_animated_icon() const;
	bool has_banner() const;
	bool is_welcome_screen_enabled() const;
	bool has_member_verification_gate() const;
	bool is_preview_enabled() const;

};

typedef std::unordered_map<snowflake, guild*> guild_map;

enum guild_member_flags {
	gm_deaf =		0b00001,
	gm_mute =		0b00010,
	gm_pending =		0b00100
};

class guild_member {
public:
	snowflake guild_id;
	snowflake user_id;
	std::string nickname;
	std::vector<snowflake> roles;
	time_t joined_at;
	time_t premium_since;
	uint8_t flags;

	guild_member();
	~guild_member();
	void fill_from_json(nlohmann::json* j, const class guild* g, const class user* u);
};

};

