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
#pragma once
#include <dpp/export.h>
#include <dpp/snowflake.h>
#include <dpp/emoji.h>
#include <dpp/json_fwd.hpp>
#include <unordered_map>

namespace dpp {

/**
 * @brief Presence flags bitmask
 */
enum presence_flags {
	/// Desktop: Online
	p_desktop_online	=	0b00000001,
	/// Desktop: DND
	p_desktop_dnd		=	0b00000010,
	/// Desktop: Idle
	p_desktop_idle		=	0b00000011,
	/// Web: Online
	p_web_online		=	0b00000100,
	/// Web: DND
	p_web_dnd		=	0b00001000,
	/// Web: Idle
	p_web_idle		=	0b00001100,
	/// Mobile: Online
	p_mobile_online		=	0b00010000,
	/// Mobile: DND
	p_mobile_dnd		=	0b00100000,
	/// Mobile: Idle
	p_mobile_idle		=	0b00110000,
	/// General: Online
	p_status_online		=	0b01000000,
	/// General: DND
	p_status_dnd		=	0b10000000,
	/// General: Idle
	p_status_idle		=	0b11000000
};

/**
 * @brief Online presence status values
 */
enum presence_status : uint8_t {
	/// Offline
	ps_offline	=	0,
	/// Online
	ps_online	=	1,
	/// DND
	ps_dnd		=	2,
	/// Idle
	ps_idle		=	3
};

/**
 * @brief Bit shift for desktop status
 */
#define PF_SHIFT_DESKTOP	0
/** Bit shift for web status */
#define PF_SHIFT_WEB		2
/** Bit shift for mobile status */
#define PF_SHIFT_MOBILE		4
/** Bit shift for main status */
#define PF_SHIFT_MAIN		6
/** Bit mask for status */
#define PF_STATUS_MASK		0b00000011
/** Bit mask for clearing desktop status */
#define PF_CLEAR_DESKTOP	0b11111100
/** Bit mask for clearing web status */
#define PF_CLEAR_WEB		0b11110011
/** Bit mask for clearing mobile status */
#define PF_CLEAR_MOBILE		0b11001111
/** Bit mask for clearing main status */
#define PF_CLEAR_STATUS		0b00111111

/**
 * @brief Game types
 */
enum activity_type : uint8_t {
	/// "Playing ..."
	at_game		=	0,
	/// "Streaming ..."
	at_streaming	=	1,
	/// "Listening to..."
	at_listening	=	2,
	/// "Watching..."
	at_watching	=	3,
	/// "Emoji..."
	at_custom	=	4,
	/// "Competing in..."
	at_competing	=	5
};

/**
 * @brief Activity types for rich presence
 */
enum activity_flags {
	/// In an instance
	af_instance						= 0b000000001,
	/// Joining
	af_join							= 0b000000010,
	/// Spectating
	af_spectate						= 0b000000100,
	/// Sending join request
	af_join_request					= 0b000001000,
	/// Synchronising
	af_sync							= 0b000010000,
	/// Playing
	af_play							= 0b000100000,
	/// Party privacy friends
	af_party_privacy_friends 		= 0b001000000,
	/// Party privacy voice channel
	af_party_privacy_voice_channel 	= 0b010000000,
	/// Embedded
	af_embedded 					= 0b100000000
};

/**
 * @brief An activity button is a custom button shown in the rich presence. Can be to join a game or whatever
 */
struct DPP_EXPORT activity_button {
public:
	/** The text shown on the button (1-32 characters)
	 */
	std::string label;
	/** The url opened when clicking the button (1-512 characters). It's may be empty
	 *
	 * @note Bots cannot access the activity button URLs.
	 */
	std::string url;

	/** Constructor */
	activity_button() = default;
};

/**
 * @brief An activity asset are the images and the hover text displayed in the rich presence
 */
struct DPP_EXPORT activity_assets {
public:
	/** The large asset image which usually contain snowflake ID or prefixed image ID
	 */
	std::string large_image;
	/** Text displayed when hovering over the large image of the activity
	 */
	std::string large_text;
	/** The small asset image which usually contain snowflake ID or prefixed image ID
	 */
	std::string small_image;
	/** Text displayed when hovering over the small image of the activity
	 */
	std::string small_text;

	/** Constructor */
	activity_assets() = default;
};

/**
 * @brief Secrets for Rich Presence joining and spectating
 */
struct DPP_EXPORT activity_secrets {
public:
	/** The secret for joining a party
	 */
	std::string join;
	/** The secret for spectating a game
	 */
	std::string spectate;
	/** The secret for a specific instanced match
	 */
	std::string match;

	/** Constructor */
	activity_secrets() = default;
};

/**
 * @brief Information for the current party of the player
 */
struct DPP_EXPORT activity_party {
public:
	/** The ID of the party
	 */
	snowflake id;
	/** The party's current size. Used to show the party's current size
	 */
	int32_t current_size;
	/** The party's maximum size. Used to show the party's maximum size
	 */
	int32_t maximum_size;

	/** Constructor */
	activity_party();
};

/**
 * @brief An activity is a representation of what a user is doing. It might be a game, or a website, or a movie. Whatever.
 */
class DPP_EXPORT activity {
public:
	/** Name of activity
	 * e.g. "Fortnite"
	 */
	std::string name;
	/** State of activity or the custom user status.
	 * e.g. "Waiting in lobby"
	 */
	std::string state;
	/** What the player is currently doing
	 */
	std::string details;
	/** Images for the presence and their hover texts
	 */
	activity_assets assets;
	/** URL.
	 * Only applicable for certain sites such a YouTube
	 * Alias: details
	 */
	std::string url;
	/** The custom buttons shown in the Rich Presence (max 2)
	 */
	std::vector<activity_button> buttons;
	/** The emoji used for the custom status
	 */
	dpp::emoji emoji;
	/** Information of the current party if there is one
	 */
	activity_party party;
	/** Secrets for rich presence joining and spectating
	 */
	activity_secrets secrets;
	/** Activity type
	 */
	activity_type type;
	/** Time activity was created
	 */
	time_t created_at;
	/** Start time. e.g. when game was started
	 */
	time_t start;
	/** End time, e.g. for songs on spotify
	 */
	time_t end;
	/** Creating application (e.g. a linked account on the user's client)
	 */
	snowflake application_id;
	/** Flags bitmask from dpp::activity_flags
	 */
	uint8_t flags;
	/** Whether or not the activity is an instanced game session
	 */
	bool is_instance;

	/**
	 * @brief Get the assets large image url if they have one, otherwise returns an empty string. In case of prefixed image IDs (mp:{image_id}) it returns an empty string.
	 *
	 * @param size The size of the image in pixels. It can be any power of two between 16 and 4096. if not specified, the default sized image is returned.
	 * @return image url or empty string
	 */
	std::string get_large_asset_url(uint16_t size = 0) const;

	/**
	 * @brief Get the assets small image url if they have one, otherwise returns an empty string. In case of prefixed image IDs (mp:{image_id}) it returns an empty string.
	 *
	 * @param size The size of the image in pixels. It can be any power of two between 16 and 4096. if not specified, the default sized image is returned.
	 * @return image url or empty string
	 */
	std::string get_small_asset_url(uint16_t size = 0) const;

	activity();

	/**
	 * @brief Construct a new activity
	 * 
	 * @param typ activity type
	 * @param nam Name of the activity
	 * @param stat State of the activity
	 * @param url_ url of the activity, only works for certain sites, such as YouTube
	 */
	activity(const activity_type typ, const std::string& nam, const std::string& stat, const std::string& url_);
};

/**
 * @brief Represents user presence, e.g. what game they are playing and if they are online
 */
class DPP_EXPORT presence {
public:
	/** The user the presence applies to */
	snowflake	user_id;

	/** Guild ID. Apparently, Discord supports this internally but the client doesnt... */
	snowflake       guild_id;

	/** Flags bitmask containing dpp::presence_flags */
	uint8_t		flags;

	/** List of activities */
	std::vector<activity> activities;

	/** Constructor */
	presence();

	/**
	 * @brief Construct a new presence object with some parameters for sending to a websocket
	 * 
	 * @param status Status of the activity
	 * @param type Type of activity
	 * @param activity_description Description of the activity
	 */
	presence(presence_status status, activity_type type, const std::string& activity_description);

	/**
	 * @brief Construct a new presence object with some parameters for sending to a websocket.
	 * 
	 * @param status Status of the activity
	 * @param a Activity itself 
	 */
	presence(presence_status status, const activity& a);

	/** Destructor */
	~presence();

	/** Fill this object from json.
	 * @param j JSON object to fill from
	 * @return A reference to self
	 */
	presence& fill_from_json(nlohmann::json* j);

	/** Build JSON from this object.
	 * 
	 * Note: This excludes any part of the presence object that are not valid for websockets and bots,
	 * and includes websocket opcode 3. You will not get what you expect if you call this on a user's
	 * presence received from on_presence_update or on_guild_create!
	 * 
	 * @return The JSON text of the presence
	 */
	std::string build_json() const;

	/** The users status on desktop
	 * @return The user's status on desktop
	 */
	presence_status desktop_status() const;

	/** The user's status on web
	 * @return The user's status on web
	 */
	presence_status web_status() const;

	/** The user's status on mobile
	 * @return The user's status on mobile
	 */
	presence_status mobile_status() const;

	/** The user's status as shown to other users
	 * @return The user's status as shown to other users
	 */
	presence_status status() const;
};

/** A container of presences */
typedef std::unordered_map<snowflake, presence> presence_map;

};
