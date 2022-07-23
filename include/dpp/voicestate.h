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
#include <dpp/nlohmann/json_fwd.hpp>
#include <unordered_map>
#include <dpp/json_interface.h>

namespace dpp {

/**
 * @brief Bit mask flags relating to voice states
 */
enum voicestate_flags {
	vs_deaf		=	0b00000001,	//!< Deafened
	vs_mute		=	0b00000010,	//!< Muted
	vs_self_mute	=	0b00000100,	//!< Self Muted
	vs_self_deaf	=	0b00001000,	//!< Self Deafened
	vs_self_stream	=	0b00010000,	//!< Self Streaming
	vs_self_video	=	0b00100000,	//!< Self Video
	vs_suppress	=	0b01000000	//!< Suppression
};

/**
 * @brief Represents the voice state of a user on a guild
 * These are stored in the dpp::guild object, and accessible there,
 * or via dpp::channel::get_voice_members
 */
class DPP_EXPORT voicestate : public json_interface<voicestate> {
public:
	class discord_client*	shard;             //!< Owning shard
	snowflake		guild_id;          //!< Optional: the guild id this voice state is for
	snowflake		channel_id;        //!< the channel id this user is connected to (may be empty)
	snowflake		user_id;           //!< the user id this voice state is for
	std::string		session_id;        //!< the session id for this voice state
	uint8_t			flags;             //!< Voice state flags
	time_t			request_to_speak;  //!< Time requested to speak, or 0

	/**
	 * @brief Construct a new voicestate object
	 */
	voicestate();

	/**
	 * @brief Destroy the voicestate object
	 */
	~voicestate();

	/**
	 * @brief Fill voicestate object from json data
	 * 
	 * @param j JSON data to fill from
	 * @return voicestate& Reference to self
	 */
	voicestate& fill_from_json(nlohmann::json* j);

	/**
	 * @brief Build json representation of the object
	 * 
	 * @param with_id Add ID to output
	 * @return std::string JSON string
	 */
	virtual std::string build_json(bool with_id = false) const;

	/// Return true if user is deafened
	bool is_deaf() const;

	/// Return true if user is muted
	bool is_mute() const;

	/// Return true if user muted themselves
	bool is_self_mute() const;

	/// Return true if user deafened themselves
	bool is_self_deaf() const;

	/// Return true if the user is streaming
	bool self_stream() const;

	/// Return true if the user is in video
	bool self_video() const;

	/// Return true if user is suppressed.
	/// "HELP HELP I'M BEING SUPPRESSED!"
	bool is_suppressed() const;
};

/** A container of voicestates */
typedef std::unordered_map<std::string, voicestate> voicestate_map;

};
