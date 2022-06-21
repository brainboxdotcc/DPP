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
#include <dpp/utility.h>
#include <dpp/nlohmann/json_fwd.hpp>
#include <dpp/json_interface.h>

namespace dpp {

enum automod_preset_type : uint8_t {
	amod_preset_profanity = 1,
	amod_preset_sexual_content = 2,
	amod_preset_slurs = 3,
};

enum automod_action_type : uint8_t {
	amod_action_block_message = 1,
	amod_action_send_alert = 2,
	amod_action_timeout = 3,
};

enum automod_event_type : uint8_t {
	/// Trigger on message send
	amod_message_send = 1,
};

enum automod_trigger_type : uint8_t {
	amod_type_keyword = 1,
	amod_type_harmful_link = 2,
	amod_type_spam = 3,
	amod_type_keyword_preset = 4,
};

struct DPP_EXPORT automod_metadata : public json_interface<automod_metadata> {
	std::vector<std::string> keywords;
	std::vector<automod_preset_type> presets;

	virtual ~automod_metadata();

	/**
	 * @brief Fill object properties from JSON
	 *
	 * @param j JSON to fill from
	 * @return automod_metadata& Reference to self
	 */
	automod_metadata& fill_from_json(nlohmann::json* j);

	/**
	 * @brief Build a json string for this object
	 *
	 * @return std::string JSON string
	 */
	virtual std::string build_json(bool with_id = false) const;

};

struct DPP_EXPORT automod_action : public json_interface<automod_action> {
	automod_action_type type;
	snowflake channel_id;
	int32_t duration_seconds;

	automod_action();

	virtual ~automod_action();

	/**
	 * @brief Fill object properties from JSON
	 *
	 * @param j JSON to fill from
	 * @return automod_action& Reference to self
	 */
	automod_action& fill_from_json(nlohmann::json* j);

	/**
	 * @brief Build a json string for this object
	 *
	 * @return std::string JSON string
	 */
	virtual std::string build_json(bool with_id = false) const;
};

class DPP_EXPORT automod_rule : public managed, public json_interface<automod_rule> {
public:
	snowflake       	id;		//!< the id of this rule
	snowflake       	guild_id;	//!< the guild which this rule belongs to
	std::string     	name;		//!< the rule name
	snowflake       	creator_id;	//!< the user which first created this rule
	automod_event_type	event_type;	//!< the rule event type
	automod_trigger_type	trigger_type;	//!< the rule trigger type
	automod_metadata	trigger_metadata;//!< the rule trigger metadata
	std::vector<automod_action> actions;	//!< the actions which will execute when the rule is triggered
	bool			enabled;	//!< whether the rule is enabled
	std::vector<snowflake>	exempt_roles;	//!< the role ids that should not be affected by the rule (Maximum of 20)
	std::vector<snowflake>	exempt_channels;//!< the channel ids that should not be affected by the rule (Maximum of 50)

	automod_rule();

	virtual ~automod_rule();

	/**
	 * @brief Fill object properties from JSON
	 *
	 * @param j JSON to fill from
	 * @return automod_rule& Reference to self
	 */
	automod_rule& fill_from_json(nlohmann::json* j);

	/**
	 * @brief Build a json string for this object
	 *
	 * @return std::string JSON string
	 */
	virtual std::string build_json(bool with_id = false) const;
};

/** A group of automod rules.
 */
typedef std::unordered_map<snowflake, automod_rule> automod_rule_map;

};
