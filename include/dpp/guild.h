#pragma once

namespace dpp {

/** Represents voice regions for guilds and channels */
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

/** The various flags that represent the status of a dpp::guild object */
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

/** Represents a guild object */
class guild : public managed {
public:
	/** Flags bitmask as defined by values within dpp::guild_flags */
	uint32_t flags;
	/** Guild name */
	std::string name;
	/** Guild icon hash */
	std::string icon;
	/** Guild splash hash */
	std::string splash;
	/** Guild discovery splash hash */
	std::string discovery_splash;
	/** Snowflake id of guild owner */
	snowflake owner_id;
	/** Guild voice region */
	region voice_region;
	/** Snowflake ID of AFK voice channel or 0 */
	snowflake afk_channel_id;
	/** Voice AFK timeout before moving users to AFK channel */
	uint32_t afk_timeout;
	/** Snowflake ID of widget channel, or 0 */
	snowflake widget_channel_id;
	/** Verification level of server */
	uint8_t verification_level;
	/** Setting for how notifications are to be delivered to users */
	uint8_t default_message_notifications;
	/** Wether or not explicit content filtering is enable and what setting it is */
	uint8_t explicit_content_filter;
	/** If multi factor authentication is required for moderators or not */
	uint8_t mfa_level;
	/** ID of creating application, if any, or 0 */
	snowflake application_id;
	/** ID of system channel where discord update messages are sent */
	snowflake system_channel_id;
	/** ID of rules channel for communities */
	snowflake rules_channel_id;
	/** Approximate member count. May be sent as zero */
	uint32_t member_count;
	/** Vanity url code for verified or partnered servers and boost level 3 */
	std::string vanity_url_code;
	/** Server description for communities */
	std::string description;
	/** Server banner hash */
	std::string banner;
	/** Boost level */
	uint8_t premium_tier;
	/** Number of boosters */
	uint16_t premium_subscription_count;
	/** Public updates channel id or 0 */
	snowflake public_updates_channel_id;
	/** Maximum users in a video channel, or 0 */
	uint32_t max_video_channel_users;

	/** Roles defined on this server */
	std::vector<snowflake> roles;

	/** List of channels on this server */
	std::vector<snowflake> channels;

	/** List of guild members. Note that when you first receive the
	 * guild create event, this may be empty or near empty.
	 * This depends upon your dpp::intents and the size of your bot.
	 * It will be filled by guild member chunk requests.
	 */
	std::map<snowflake, class guild_member*> members;

	/** List of emojis
	 */
	std::map<snowflake, class emoji*> emojis;

	/** Default constructor, zeroes all values */
	guild();

	/** Destructor */
	~guild();

	/** Build this object from json data.
	 * @param j json data
	 */
	guild& fill_from_json(nlohmann::json* j);

	/** Build a JSON string from this object.
	 * @param with_id True if an ID is to be included in the JSON
	 */
	std::string build_json(bool with_id = false) const;

	/** Is a large server (>250 users) */
	bool is_large() const;

	/** Is unavailable due to outage (most other fields will be blank or outdated */
	bool is_unavailable() const;

	/** Widget is enabled for this server */
	bool widget_enabled() const;

	/** Guild has an invite splash */
	bool has_invite_splash() const;

	/** Guild has VIP regions */
	bool has_vip_regions() const;

	/** Guild can have a vanity url */
	bool has_vanity_url() const;

	/** Guild is a verified server */
	bool is_verified() const;

	/** Guild is a discord partner server */
	bool is_partnered() const;

	/** Guild has enabled community */
	bool is_community() const;

	/** Guild has enabled commerce channels */
	bool has_commerce() const;

	/** Guild has news channels */
	bool has_news() const;

	/** Guild is discoverable */
	bool is_discoverable() const;

	/** Guild is featureable */
	bool is_featureable() const;

	/** Guild has an animated icon */
	bool has_animated_icon() const;

	/** Guild has a banner image */
	bool has_banner() const;

	/** Guild has enabled welcome screen */
	bool is_welcome_screen_enabled() const;

	/** Guild has enabled membership screening */
	bool has_member_verification_gate() const;

	/** Guild has preview enabled */
	bool is_preview_enabled() const;

};

/** A container of guilds */
typedef std::unordered_map<snowflake, guild> guild_map;

/** Various flags that can be used to indicate the status of a guild member */
enum guild_member_flags {
	/** Member deafened */
	gm_deaf =		0b00001,
	/** Member muted */
	gm_mute =		0b00010,
	/** Member pending verification by membership screening */
	gm_pending =		0b00100
};

/** Represents dpp::user membership upon a dpp::guild */
class guild_member {
public:
	/** Guild id */
	snowflake guild_id;
	/** User id */
	snowflake user_id;
	/** Nickname, or empty string if they don't have a nickname on this guild */
	std::string nickname;
	/** List of roles this user has on this guild */
	std::vector<snowflake> roles;
	/** Date and time the user joined the guild */
	time_t joined_at;
	/** Boosting since */
	time_t premium_since;
	/** A set of flags built from the bitmask defined by dpp::guild_member_flags */
	uint8_t flags;

	/** Default constructor */
	guild_member();

	/** Default destructor */
	~guild_member();

	/** Fill this object from a json object.
	 * @param j The json object to get data from
	 * @param g The guild to associate the member with
	 * @param u The user to associate the member with
	 */
	guild_member& fill_from_json(nlohmann::json* j, const class guild* g, const class user* u);
};

/** A container of guild members */
typedef std::unordered_map<snowflake, guild_member> guild_member_map;


};

