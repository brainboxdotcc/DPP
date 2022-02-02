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
#include <dpp/managed.h>
#include <dpp/user.h>
#include <dpp/guild.h>
#include <dpp/json_fwd.hpp>

namespace dpp {

/**
 * @brief Represents the privacy of an event
 */
enum event_privacy_level : uint8_t {
	/// The event is visible to only guild members.
	ep_guild_only = 2
};

/**
 * @brief Event entity types
 */
enum event_entity_type : uint8_t {
	/// A stage instance
	eet_stage_instance = 1,
	/// A voice channel
	eet_voice = 2,
	/// External to discord, or a text channel etc
	eet_external = 3
};

/**
 * @brief Event status types
 */
enum event_status : uint8_t {
	/// Scheduled
	es_scheduled	=	1,
	/// Active now
	es_active	=	2,
	/// Completed
	es_completed	=	3,
	/// Cancelled
	es_cancelled	=	4
};

/**
 * @brief Entities for the event
 */
struct DPP_EXPORT event_entities {
	/// location of the event
	std::string location;
};

/**
 * @brief Represents a guild member/user who has registered interest in an event
 * 
 */
struct DPP_EXPORT event_member {
	/**
	 * @brief Event ID associated with
	 */
        snowflake guild_scheduled_event_id;
	/**
	 * @brief User details of associated user
	 * 
	 */
        dpp::user user;
	/**
	 * @brief Member details of user on the associated guild
	 */
        dpp::guild_member member;
};

/**
 * @brief A scheduled event
 */
struct DPP_EXPORT scheduled_event : public managed {
	snowflake		guild_id;		//!< the guild id which the scheduled event belongs to
	snowflake		channel_id;		//!< the channel id in which the scheduled event will be hosted, or null if scheduled entity type is EXTERNAL (may be empty)
	snowflake		creator_id;		//!< Optional: the id of the user that created the scheduled event
	std::string		name;			//!< the name of the scheduled event
	std::string		description;		//!< Optional: the description of the scheduled event
	std::string		image;			//!< the image of the scheduled event (may be empty)
	time_t			scheduled_start_time;	//!< the time the scheduled event will start
	time_t			scheduled_end_time;	//!< the time the scheduled event will end, or null if the event does not have a scheduled time to end (may be empty)
	event_privacy_level	privacy_level;		//!< the privacy level of the scheduled event
	event_status		status;			//!< the status of the scheduled event
	event_entity_type	entity_type;		//!< the type of hosting entity associated with a scheduled event, e.g. voice channel or stage channel
	snowflake		entity_id;		//!< any additional id of the hosting entity associated with event, e.g. stage instance id) (may be empty)
	event_entities		entity_metadata;	//!< the entity metadata for the scheduled event (may be empty)
	user			creator;		//!< Optional: the creator of the scheduled event
	uint32_t		user_count;		//!< Optional: the number of users subscribed to the scheduled event

	/**
	 * @brief Create a scheduled_event object
	 */
	scheduled_event();

	/**
	 * @brief Destroy the scheduled_event object
	 */
	~scheduled_event() = default;

	/**
	 * @brief Set the name of the event
	 * Minimum length: 1, Maximum length: 100
	 * @param n event name
	 * @return scheduled_event& reference to self
	 * @throw dpp::length_error if length < 1
	 */
	scheduled_event& set_name(const std::string& n);

	/**
	 * @brief Set the description of the event
	 * Minimum length: 1 (if set), Maximum length: 100
	 * @param d event description
	 * @return scheduled_event& reference to self
	 * @throw dpp::length_error if length < 1
	 */
	scheduled_event& set_description(const std::string& d);

	/**
	 * @brief Clear the description of the event
	 * @return scheduled_event& reference to self
	 */
	scheduled_event& clear_description();

	/**
	 * @brief Set the location of the event.
	 * Minimum length: 1, Maximum length: 1000
	 * @note Clears channel_id
	 * @param l event location
	 * @return scheduled_event& reference to self
	 * @throw dpp::length_error if length < 1
	 */
	scheduled_event& set_location(const std::string& l);

	/**
	 * @brief Set the voice channel id of the event
	 * @note clears location
	 * @param c channel ID
	 * @return scheduled_event& reference to self
	 */
	scheduled_event& set_channel_id(snowflake c);

	/**
	 * @brief Set the creator id of the event
	 * @param c creator user ID
	 * @return scheduled_event& reference to self
	 */
	scheduled_event& set_creator_id(snowflake c);

	/**
	 * @brief Set the status of the event
	 * @param s status to set
	 * @return scheduled_event& reference to self
	 * @throw dpp::logic_error if status change is not valid
	 */
	scheduled_event& set_status(event_status s);

	/**
	 * @brief Set the start time of the event
	 * @param t starting time
	 * @return scheduled_event& reference to self
	 * @throw dpp::length_error if time is before now
	 */
	scheduled_event& set_start_time(time_t t);

	/**
	 * @brief Set the end time of the event
	 * @param t ending time
	 * @return scheduled_event& reference to self
	 * @throw dpp::length_error if time is before now
	 */
	scheduled_event& set_end_time(time_t t);

	/**
	 * @brief Serialise a scheduled_event object from json
	 *
	 * @return scheduled_event& a reference to self
	 */
	scheduled_event& fill_from_json(const nlohmann::json* j);

	/**
	 * @brief Build json for this object
	 * @param with_id Include id field in json
	 *
	 * @return std::string Dumped json of this object
	 */
	std::string const build_json(bool with_id = false) const;
};

/**
 * @brief A group of scheduled events
 */
typedef std::unordered_map<snowflake, scheduled_event> scheduled_event_map;

/**
 * @brief A group of scheduled event members
 */
typedef std::unordered_map<snowflake, event_member> event_member_map;


};
