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

#include <dpp/discord.h>
#include <dpp/json_fwd.hpp>

namespace dpp {

/**
 * @brief Presence flags bitmask
 */
enum presence_flags {
	p_desktop_online	=	0b00000001,
	p_desktop_dnd		=	0b00000010,
	p_desktop_idle		=	0b00000011,
	p_web_online		=	0b00000100,
	p_web_dnd		=	0b00001000,
	p_web_idle		=	0b00001100,
	p_mobile_online		=	0b00010000,
	p_mobile_dnd		=	0b00100000,
	p_mobile_idle		=	0b00110000,
	p_status_online		=	0b01000000,
	p_status_dnd		=	0b10000000,
	p_status_idle		=	0b11000000
};

/**
 * @brief Online presence status values
 */
enum presence_status : uint8_t {
	ps_offline	=	0,
	ps_online	=	1,
	ps_dnd		=	2,
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
	at_game		=	0, //< "Playing ..."
	at_streaming	=	1, //< "Streaming ..."
	at_listening	=	2, //< "Listening to..."
	at_custom	=	3, //< "Watching..."
	at_competing	=	4  //< "Competing in..."
};

/**
 * @brief Activity types for rich presence
 */
enum activity_flags {
	af_instance	= 0b00000001,
	af_join		= 0b00000010,
	af_spectate	= 0b00000100,
	af_join_request	= 0b00001000,
	af_sync		= 0b00010000,
	af_play		= 0b00100000
};

/**
 * @brief An activity is a representation of what a user is doing. It might be a game, or a website, or a movie. Whatever.
 */
class activity {
public:
	/** Name of ativity
	 * e.g. "Fortnite"
	 */
	std::string name;
	/** State of activity.
	 * e.g. "Waiting in lobby"
	 */
	std::string state;
	/** URL.
	 * Only applicable for certain sites such a YouTube
	 */
	std::string url;
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
	/** Flags bitmask from activity_flags
	 */
	uint8_t flags;
};

/**
 * @brief Represents user presence, e.g. what game they are playing and if they are online
 */
class presence {
public:
	/** The user the presence applies to */
	snowflake	user_id;

	/** Guild ID. Apparently, Discord supports this internally but the client doesnt... */
	snowflake       guild_id;

	/** Flags bitmask containing presence_flags */
	uint8_t		flags;

	/** List of activities */
	std::vector<activity> activities;

	/** Constructor */
	presence();

	/**
	 * @brief Construct a new presence object with some parameters for sending to a websocket
	 * 
	 * @param status 
	 * @param type 
	 * @param activity_description 
	 */
	presence(presence_status status, activity_type type, const std::string activity_description);

	/** Destructor */
	~presence();

	/** Fill this object from json.
	 * @param j JSON object to fill from
	 * @return A reference to self
	 */
	presence& fill_from_json(nlohmann::json* j);

	/** Build JSON from this object.
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
typedef std::unordered_map<std::string, presence> presence_map;

};
