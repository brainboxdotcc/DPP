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
#include <dpp/nlohmann/json_fwd.hpp>
#include <dpp/snowflake.h>
#include <dpp/managed.h>
#include <dpp/utility.h>
#include <dpp/json_interface.h>

namespace dpp {

/**
 * @brief Various bitmask flags used to represent information about a dpp::user
 */
enum user_flags : uint32_t {
	/// User is a bot
	u_bot =				0b00000000000000000000001,
	/// User is a system user (Clyde!)
	u_system =			0b00000000000000000000010,
	/// User has multi-factor authentication enabled
	u_mfa_enabled =			0b00000000000000000000100,
	/// User is verified (verified email address)
	u_verified =			0b00000000000000000001000,
	/// User has full nitro
	u_nitro_full =			0b00000000000000000010000,
	/// User has nitro classic
	u_nitro_classic =		0b00000000000000000100000,
	/// User is discord staff
	u_discord_employee =		0b00000000000000001000000,
	/// User owns a partnered server
	u_partnered_owner =		0b00000000000000010000000,
	/// User is a member of hypesquad events
	u_hypesquad_events =		0b00000000000000100000000,
	/// User has BugHunter level 1
	u_bughunter_1 =			0b00000000000001000000000,
	/// User is a member of House Bravery
	u_house_bravery =		0b00000000000010000000000,
	/// User is a member of House Brilliance
	u_house_brilliance =		0b00000000000100000000000,
	/// User is a member of House Balance
	u_house_balance =		0b00000000001000000000000,
	/// User is an early supporter
	u_early_supporter =		0b00000000010000000000000,
	/// User is a team user
	u_team_user =			0b00000000100000000000000,
	/// User is has Bug Hunter level 2
	u_bughunter_2 =			0b00000001000000000000000,
	/// User is a verified bot
	u_verified_bot =		0b00000010000000000000000,
	/// User has the Early Verified Bot Developer badge
	u_verified_bot_dev =	 	0b00000100000000000000000,
	/// User's icon is animated
	u_animated_icon =		0b00001000000000000000000,
	/// User is a certified moderator
	u_certified_moderator =		0b00010000000000000000000,
	/// User is a bot using HTTP interactions (shows online even when not connected to a websocket)
	u_bot_http_interactions =	0b00100000000000000000000,
};

/**
 * @brief Represents a user on discord. May or may not be a member of a dpp::guild.
 */
class DPP_EXPORT user : public managed, public json_interface<user>  {
public:
	/** Discord username */
	std::string username;
	/** Avatar hash */
	utility::iconhash avatar;
	/** Flags built from a bitmask of values in dpp::user_flags */
	uint32_t flags;
	/** Discriminator (aka tag), 4 digits usually displayed with leading zeroes.
	 *
	 * @note To print the discriminator with leading zeroes, use something like `fmt::format("{:04d}", discriminator)`
	 */
	uint16_t discriminator;
	/** Reference count of how many guilds this user is in */
	uint8_t refcount;

	/**
	 * @brief Construct a new user object
	 */
	user();

	/**
	 * @brief Destroy the user object
	 */
	virtual ~user();

	/** Fill this record from json.
	 * @param j The json to fill this record from
	 * @return Reference to self
	 */
	user& fill_from_json(nlohmann::json* j);

	/**
	 * @brief Convert to JSON string
	 * 
	 * @param with_id include ID in output
	 * @return std::string JSON output
	 */
	virtual std::string build_json(bool with_id = true) const;

	/**
	 * @brief Get the avatar url of the user object
	 *
	 * @param size The size of the avatar in pixels. It can be any power of two between 16 and 4096. if not specified, the default sized avatar is returned.
	 * @return std::string avatar url. If the user doesn't have an avatar, the default user avatar url is returned
	 */
	std::string get_avatar_url(uint16_t size = 0) const;

	/**
	 * @brief Return a ping/mention for the user
	 * 
	 * @return std::string mention
	 */
	std::string get_mention() const;

	/**
	 * @brief User is a bot
	 *
	 * @return True if the user is a bot
	 */
	bool is_bot() const;
	/**
	 * @brief User is a system user (Clyde)
	 *
	 * @return true  if user is a system user
	 */
	bool is_system() const;
	/**
	 * @brief User has multi-factor authentication enabled
	 *
	 * @return true if multi-factor is enabled
	 */
	bool is_mfa_enabled() const;
	/**
	 * @brief Return true if user has verified account
	 *
	 * @return true if verified
	 */
	bool is_verified() const;
	/**
	 * @brief Return true if user has full nitro.
	 * This is mutually exclusive with full nitro.
	 *
	 * @return true if user has full nitro
	 */
	bool has_nitro_full() const;
	/**
	 * @brief Return true if user has nitro classic.
	 * This is mutually exclusive with nitro classic.
	 *
	 * @return true  if user has nitro classic
	 */
	bool has_nitro_classic() const;
	/**
	 * @brief Return true if user is a discord employee
	 *
	 * @return true if user is discord staff
	 */
	bool is_discord_employee() const;
	/**
	 * @brief Return true if user owns a partnered server
	 *
	 * @return true if user has partnered server
	 */
	bool is_partnered_owner() const;
	/**
	 * @brief Return true if user has hypesquad events
	 *
	 * @return true if has hypesquad events
	 */
	bool has_hypesquad_events() const;
	/**
	 * @brief Return true if user has the bughunter level 1 badge
	 *
	 * @return true if has bughunter level 1
	 */
	bool is_bughunter_1() const;
	/**
	 * @brief Return true if user is in house bravery
	 *
	 * @return true if in house bravery
	 */
	bool is_house_bravery() const;
	/**
	 * @brief Return true if user is in house brilliance
	 *
	 * @return true if in house brilliance
	 */
	bool is_house_brilliance() const;
	/**
	 * @brief Return true if user is in house balance
	 *
	 * @return true if in house brilliance
	 */
	bool is_house_balance() const;
	/**
	 * @brief Return true if user is an early supporter
	 *
	 * @return true if early supporter
	 */
	bool is_early_supporter() const;
	/**
	 * @brief Return true if user is a team user
	 *
	 * @return true if a team user
	 */
	bool is_team_user() const;
	/**
	 * @brief Return true if user has the bughunter level 2 badge
	 *
	 * @return true if has bughunter level 2
	 */
	bool is_bughunter_2() const;
	/**
	 * @brief Return true if user has the verified bot badge
	 *
	 * @return true if verified bot
	 */
	bool is_verified_bot() const;
	/**
	 * @brief Return true if user is an early verified bot developer
	 *
	 * @return true if verified bot developer
	 */
	bool is_verified_bot_dev() const;
	/**
	 * @brief Return true if user is a certified moderator
	 *
	 * @return true if certified moderator
	 */
	bool is_certified_moderator() const;
	/**
	 * @brief Return true if user is a bot which exclusively uses HTTP interactions.
	 * Bots using HTTP interactions are always considered online even when not connected to a websocket.
	 *
	 * @return true if is a http interactions only bot
	 */
	bool is_bot_http_interactions() const;
	/**
	 * @brief Return true if user has an animated icon
	 *
	 * @return true if icon is animated (gif)
	 */
	bool has_animated_icon() const;

	/**
	 * @brief Format a username into user#discriminator
	 * 
	 * For example Brain#0001
	 * 
	 * @return Formatted username and discriminator
	 */
	std::string format_username() const;
};

/**
 * @brief A user with additional fields only available via the oauth2 identify scope.
 * These are not included in dpp::user as additional scopes are needed to fetch them
 * which bots do not normally have.
 */
class DPP_EXPORT user_identified : public user, public json_interface<user_identified> {
public:
	std::string		locale;		//!< Optional: the user's chosen language option identify
	std::string		email;		//!< Optional: the user's email  email (may be empty)
	utility::iconhash	banner;		//!< Optional: the user's banner hash    identify (may be empty)
	uint32_t		accent_color;	//!< Optional: the user's banner color encoded as an integer representation of hexadecimal color code    identify (may be empty)
	bool			verified;	//!< Optional: whether the email on this account has been verified       email
	
	/** Fill this record from json.
	 * @param j The json to fill this record from
	 * @return Reference to self
	 */
	user_identified& fill_from_json(nlohmann::json* j);

	/**
	 * @brief Convert to JSON string
	 * 
	 * @param with_id include ID in output
	 * @return std::string JSON output
	 */
	virtual std::string build_json(bool with_id = true) const;

	/**
	 * @brief Construct a new user identified object
	 */
	user_identified();

	/**
	 * @brief Destroy the user identified object
	 */
	virtual ~user_identified();

	/**
	 * @brief Get the user identified's banner url if they have one, otherwise returns an empty string
	 *
	 * @param size The size of the banner in pixels. It can be any power of two between 16 and 4096. if not specified, the default sized banner is returned.
	 * @return std::string banner url or empty string
	 */
	std::string get_banner_url(uint16_t size = 0) const;

};

/**
 * @brief helper function to deserialize a user from json
 *
 * @see https://github.com/nlohmann/json#arbitrary-types-conversions
 *
 * @param j output json object
 * @param u user to be deserialized
 */
void from_json(const nlohmann::json& j, user& u);

/**
 * @brief helper function to deserialize a user_identified from json
 *
 * @see https://github.com/nlohmann/json#arbitrary-types-conversions
 *
 * @param j output json object
 * @param u user to be deserialized
 */
void from_json(const nlohmann::json& j, user_identified& u);

/** A group of users */
typedef std::unordered_map<snowflake, user> user_map;

};
