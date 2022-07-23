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

/**
 * @brief Possible types of preset filter lists
 */
enum automod_preset_type : uint8_t {
	/**
	 * @brief Strong swearing
	 */
	amod_preset_profanity = 1,
	/**
	 * @brief Sexual phrases and words
	 */
	amod_preset_sexual_content = 2,
	/**
	 * @brief Racial and other slurs, hate speech
	 */
	amod_preset_slurs = 3,
};

/**
 * @brief Action types to perform on filtering
 */
enum automod_action_type : uint8_t {
	/**
	 * @brief Block the message
	 */
	amod_action_block_message = 1,
	/**
	 * @brief Send an alert to a given channel
	 */
	amod_action_send_alert = 2,
	/**
	 * @brief time out the user
	 */
	amod_action_timeout = 3,
};

/**
 * @brief Event types, only message send is currently supported
 */
enum automod_event_type : uint8_t {
	/**
	 * @brief Trigger on message send or edit
	 */
	amod_message_send = 1,
};

/**
 * @brief Types of moderation to trigger
 */
enum automod_trigger_type : uint8_t {
	/**
	 * @brief Keyword filtering
	 */
	amod_type_keyword = 1,
	/**
	 * @brief Harmful/malware links
	 */
	amod_type_harmful_link = 2,
	/**
	 * @brief Spamming
	 */
	amod_type_spam = 3,
	/**
	 * @brief Preset lists of filter words
	 */
	amod_type_keyword_preset = 4,
};

/**
 * @brief Metadata associated with an automod action
 */
struct DPP_EXPORT automod_metadata : public json_interface<automod_metadata> {
	/**
	 * @brief Keywords to moderate. A keyword can be a phrase which contains multiple words. All keywords are case insensitive.
	 * `*` can be used to customize how each keyword will be matched.
	 *
	 * **Examples for the `*` wildcard symbol:**
	 *
	 * Prefix - word must start with the keyword
	 *
	 * | keyword  | matches                             |
     * |----------|-------------------------------------|
     * | cat*     | <u><b>cat</b></u>ch, <u><b>Cat</b></u>apult, <u><b>CAt</b></u>tLE |
     * | the mat* | <u><b>the mat</b></u>rix                      |
     *
     * Suffix - word must end with the keyword
     *
     * | keyword  | matches                  |
     * |----------|--------------------------|
     * | *cat     | wild<u><b>cat</b></u>, copy<u><b>Cat</b></u> |
     * | *the mat | brea<u><b>the mat</b></u>          |
     *
     * Anywhere - keyword can appear anywhere in the content
     *
     * | keyword   | matches                     |
     * |-----------|-----------------------------|
     * | \*cat*     | lo<u><b>cat</b></u>ion, edu<u><b>Cat</b></u>ion |
     * | \*the mat* | brea<u><b>the mat</b></u>ter          |
     *
     * Whole Word - keyword is a full word or phrase and must be surrounded by whitespace at the beginning and end
     *
     * | keyword | matches     |
     * |---------|-------------|
     * | cat     | <u><b>Cat</b></u>     |
     * | the mat | <u><b>the mat</b></u> |
     *
	 */
	std::vector<std::string> keywords;
	/**
	 * @brief Preset keyword list types to moderate
	 * @see automod_preset_type
	 */
	std::vector<automod_preset_type> presets;

	/**
	 * @brief Destroy the automod metadata object
	 */
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

/**
 * @brief Represents an automod action
 */
struct DPP_EXPORT automod_action : public json_interface<automod_action> {
	/**
	 * @brief Type of action to take
	 */
	automod_action_type type;

	/**
	 * @brief Channel ID, for type amod_action_send_alert
	 */
	snowflake channel_id;

	/**
	 * @brief Silence duration in seconds (Maximum of 2419200), for amod_action_timeout
	 * 
	 */
	int32_t duration_seconds;

	/**
	 * @brief Construct a new automod action object
	 */
	automod_action();

	/**
	 * @brief Destroy the automod action object
	 */
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

/**
 * @brief Represents an automod rule
 */
class DPP_EXPORT automod_rule : public managed, public json_interface<automod_rule> {
public:
	/**
	 * @brief the id of this rule
	 */
	snowflake       	id;
	/**
	 * @brief the guild which this rule belongs to
	 */
	snowflake       	guild_id;
	/**
	 * @brief the rule name
	 */
	std::string     	name;
	/**
	 * @brief The user which first created this rule
	 */
	snowflake       	creator_id;
	/**
	 * @brief The rule event type
	 */
	automod_event_type	event_type;
	/**
	 * @brief The rule trigger type
	 */
	automod_trigger_type	trigger_type;
	/**
	 * @brief The rule trigger metadata
	 * 
	 */
	automod_metadata	trigger_metadata;
	/**
	 * @brief the actions which will execute when the rule is triggered
	 */
	std::vector<automod_action> actions;
	/**
	 * @brief Whether the rule is enabled
	 */
	bool			enabled;
	/**
	 * @brief the role ids that should not be affected by the rule (Maximum of 20)
	 */
	std::vector<snowflake>	exempt_roles;
	/**
	 * @brief the channel ids that should not be affected by the rule (Maximum of 50)
	 */
	std::vector<snowflake>	exempt_channels;

	/**
	 * @brief Construct a new automod rule object
	 */
	automod_rule();

	/**
	 * @brief Destroy the automod rule object
	 */
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
