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
#include <dpp/user.h>
#include <dpp/json_fwd.hpp>

namespace dpp {

/**
 * @brief status of a member of a team who maintain a bot/application
 */
enum team_member_status : uint8_t {
	/// User was invited to the team
	tms_invited = 1,
	/// User has accepted membership onto the team
	tms_accepted = 2
};

/**
 * @brief Flags for a bot or application
 */
enum application_flags : uint32_t {
	/// Has gateway presence intent
	apf_gateway_presence = (1 << 12),
	/// Has gateway presence intent for <100 guilds
	apf_gateway_presence_limited = (1 << 13),
	/// Has guild members intent 
	apf_gateway_guild_members = (1 << 14),
	/// Has guild members intent for <100 guilds
	apf_gateway_guild_members_limited = (1 << 15),
	/// Verification is pending
	apf_verification_pending_guild_limit = (1 << 16),
	/// Embedded
	apf_embedded = (1 << 17),
	/// Has approval for message content
	apf_gateway_message_content = (1 << 18),
	/// Has message content, but <100 guilds
	apf_gateway_message_content_limited = (1 << 19)
};

/**
 * @brief Represents a team member on a team who maintain a bot/application
 */
class DPP_EXPORT team_member {
public:
	team_member_status	membership_state;	//!< the user's membership state on the team
	std::string		permissions;		//!< will always be [""]
	snowflake		team_id;		//!< the id of the parent team of which they are a member
	user			member_user;		//!< the avatar, discriminator, id, and username of the user
};

/**
 * @brief Represents a team of users who maintain a bot/application
 */
class DPP_EXPORT app_team {
public:
	utility::iconhash		icon;		//!< a hash of the image of the team's icon (may be empty)
	snowflake			id;		//!< the unique id of the team
	std::vector<team_member>	members;	//!< the members of the team
	std::string			name;		//!< the name of the team
	snowflake			owner_user_id;	//!< the user id of the current team owner
};

/**
 * @brief The application class represents details of a bot application
 */
class DPP_EXPORT application : public managed {
public:
	std::string		name;			//!< the name of the app
	utility::iconhash	icon;			//!< the icon hash of the app (may be empty)
	std::string		description;		//!< the description of the app
	std::string		rpc_origins;		//!< Optional: an array of rpc origin urls, if rpc is enabled
	bool			bot_public;		//!< when false only app owner can join the app's bot to guilds
	bool			bot_require_code_grant;	//!< when true the app's bot will only join upon completion of the full oauth2 code grant flow
	std::string		terms_of_service_url;	//!< Optional: the url of the app's terms of service
	std::string		privacy_policy_url;	//!< Optional: the url of the app's privacy policy
	user			owner;			//!< Optional: partial user object containing info on the owner of the application
	std::string		summary;		//!< if this application is a game sold on Discord, this field will be the summary field for the store page of its primary sku
	std::string		verify_key;		//!< the hex encoded key for verification in interactions and the GameSDK's GetTicket
	app_team		team;			//!< if the application belongs to a team, this will be a list of the members of that team (may be empty)
	snowflake		guild_id;		//!< Optional: if this application is a game sold on Discord, this field will be the guild to which it has been linked
	snowflake		primary_sku_id;		//!< Optional: if this application is a game sold on Discord, this field will be the id of the "Game SKU" that is created, if exists
	std::string		slug;			//!< Optional: if this application is a game sold on Discord, this field will be the URL slug that links to the store page
	utility::iconhash	cover_image;		//!< Optional: the application's default rich presence invite cover image hash
	uint32_t		flags;			//!< Optional: the application's public flags
	
	/** Constructor */
	application();

	/** Destructor */
	~application();

	/** Read class values from json object
	 * @param j A json object to read from
	 * @return A reference to self
	 */
	application& fill_from_json(nlohmann::json* j);
};

/** A group of applications.
 * This is not currently ever sent by Discord API but the DPP standard setup for
 * objects that can be received by REST has the possibility for this, so this exists.
 * Don't ever expect to see one at present.
 */
typedef std::unordered_map<snowflake, application> application_map;

};
