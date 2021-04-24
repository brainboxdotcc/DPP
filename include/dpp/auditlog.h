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
#include <optional>

namespace dpp {

/**
 * @brief Defines types of audit log entry
 */
enum audit_type {
	ae_guild_update			=	1,
	ae_channel_create		=	10,
	ae_channel_update		=	11,
	ae_channel_delete		=	12,
	ae_channel_overwrite_create	=	13,
	ae_channel_overwrite_update	=	14,
	ae_channel_overwrite_delete	=	15,
	ae_member_kick			=	20,
	ae_member_prune			=	21,
	ae_member_ban_add		=	22,
	ae_member_ban_remove		=	23,
	ae_member_update		=	24,
	ae_member_role_update		=	25,
	ae_member_move			=	26,
	ae_member_disconnect		=	27,
	ae_bot_add			=	28,
	ae_role_create			=	30,
	ae_role_update			=	31,
	ae_role_delete			=	32,
	ae_invite_create		=	40,
	ae_invite_update		=	41,
	ae_invite_delete		=	42,
	ae_webhook_create		=	50,
	ae_webhook_update		=	51,
	ae_webhook_delete		=	52,
	ae_emoji_create			=	60,
	ae_emoji_update			=	61,
	ae_emoji_delete			=	62,
	ae_message_delete		=	72,
	ae_message_bulk_delete		=	73,
	ae_message_pin			=	74,
	ae_message_unpin		=	75,
	ae_integration_create		=	80,
	ae_integration_update		=	81,
	ae_integration_delete		=	82
};

/**
 * @brief Defines audit log changes
 */
struct audit_change {
	std::string	new_value;      //< Optional: Serialised new value of the key
	std::string	old_value;      //< Optional: Serialised old value of the key
	std::string	key;    //< name of audit log change key
};

/**
 * @brief Extra information for an audit log entry
 */
struct audit_extra {
	std::string 	delete_member_days;	//< number of days after which inactive members were kicked
	std::string	members_removed;	//< number of members removed by the prune
	snowflake	channel_id;		//< channel in which the entities were targeted
	snowflake	message_id;		//< id of the message that was targeted
	std::string	count;			//< number of entities that were targeted
	snowflake	id;			//< id of the overwritten entity
	std::string	type;			//< type of overwritten entity - "0" for "role" or "1" for "member"
	std::string 	role_name;		//< name of the role if type is "0" (not present if type is "1")
};

/**
 * @brief An individual audit log entry
 */
struct audit_entry {
	snowflake			id;		//< id of the entry
	snowflake			target_id;	//< id of the affected entity (webhook, user, role, etc.) (may be empty)
	std::vector<audit_change>	changes;	//< Optional: changes made to the target_id
	snowflake			user_id;	//< the user who made the changes (may be empty)
	audit_type			event;		//< type of action that occurred
	std::optional<audit_extra>	options;	//< Optional: additional info for certain action types
	std::string			reason;		//< Optional: the reason for the change (0-512 characters)	
};

/**
 * @brief The auditlog class represents the audit log entry of a guild.
 */
class auditlog {
public:
	std::vector<audit_entry> entries;	//< Audit log entries
	
	/** Constructor */
	auditlog();

	/** Destructor */
	~auditlog();

	/** Read class values from json object
	 * @param j A json object to read from
	 * @return A reference to self
	 */
	auditlog& fill_from_json(nlohmann::json* j);
};

};
