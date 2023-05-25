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
#include <dpp/json_fwd.h>
#include <unordered_map>
#include <dpp/json_interface.h>

namespace dpp {

/**
 * @brief Integration types
 */
enum integration_type {
	/// Twitch integration
	i_twitch,
	/// YouTube integration
	i_youtube,
	/// Discord integration
	i_discord,
	/// Subscription
	i_guild_subscription,
};

/**
 * @brief Integration flags
 */
enum integration_flags {
	/// Integration enabled
	if_enabled =     0b00000001,
	/// Integration syncing
	if_syncing =     0b00000010,
	/// Emoji integration
	if_emoticons =   0b00000100,
	/// Integration revoked
	if_revoked =     0b00001000,
	/// Kick users when their subscription expires
	if_expire_kick = 0b00010000,
};

/**
 * @brief An application that has been integrated
 */
struct DPP_EXPORT integration_app {
	/// Integration id
	snowflake id;
	/// Name
	std::string name;
	/// Icon
	std::string icon;
	/// Description
	std::string description;
	/// Integration summary @deprecated Removed by Discord
	std::string summary;
	/// Pointer to bot user
	class user* bot;
};

/**
 * @brief Represents an integration on a guild, e.g. a connection to twitch.
 */
class DPP_EXPORT integration : public managed, public json_interface<integration> {
public:
	/** Integration name */
	std::string name;
	/** Integration type */
	integration_type type;
	/** Integration flags from dpp::integration_flags */
	uint8_t flags;
	/** Role id */
	snowflake role_id;
	/** User id */
	snowflake user_id;
	/** The grace period (in days) before expiring subscribers */
	uint32_t expire_grace_period;
	/** Sync time */
	time_t synced_at;
	/** Subscriber count */
	uint32_t subscriber_count;
	/** Account id */
	std::string account_id;
	/** Account name */
	std::string account_name;
	/** The bot/OAuth2 application for discord integrations */
	integration_app app;

	/** Default constructor */
	integration();

	/** Default destructor */
	~integration();

	/** Read class values from json object
	 * @param j A json object to read from
	 * @return A reference to self
	 */
	integration& fill_from_json(nlohmann::json* j);

	/** Build a json string from this object.
	 * @param with_id Add ID to output
	 * @return JSON string of the object
	 */
	virtual std::string build_json(bool with_id = false) const;

	/** True if emoticons are enabled */
	bool emoticons_enabled() const;
	/** True if integration is enabled */
	bool is_enabled() const;
	/** True if is syncing */
	bool is_syncing() const;
	/** True if has been revoked */
	bool is_revoked() const;
	/** True if expiring kicks the user */
	bool expiry_kicks_user() const;
};

/**
 * @brief The connection object that the user has attached.
 */
class DPP_EXPORT connection {
public:
	std::string			id;		//!< id of the connection account
	std::string			name;		//!< the username of the connection account
	std::string			type;		//!< the service of the connection (twitch, youtube, discord, or guild_subscription)
	bool				revoked;	//!< Optional: whether the connection is revoked
	std::vector<integration>	integrations;	//!< Optional: an array of partial server integrations
	bool				verified;	//!< whether the connection is verified
	bool				friend_sync;	//!< whether friend sync is enabled for this connection
	bool				show_activity;	//!< whether activities related to this connection will be shown in presence updates
	bool				two_way_link;	//!< Whether this connection has a corresponding third party OAuth2 token
	bool				visible;	//!< visibility of this connection

	/**
	 * @brief Construct a new connection object
	 */
	connection();

	/** Read class values from json object
	 * @param j A json object to read from
	 * @return A reference to self
	 */
	connection& fill_from_json(nlohmann::json* j);

};

/** A group of integrations */
typedef std::unordered_map<snowflake, integration> integration_map;

/** A group of connections */
typedef std::unordered_map<snowflake, connection> connection_map;

};

