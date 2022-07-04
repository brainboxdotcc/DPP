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
#include <optional>
#include <dpp/json_interface.h>

namespace dpp {

/**
 * @brief Defines types of audit log entry
 */
enum audit_type {
	/// Guild update
	ae_guild_update			=	1,
	/// Channel create
	ae_channel_create		=	10,
	/// Channel update
	ae_channel_update		=	11,
	/// Channel delete
	ae_channel_delete		=	12,
	/// Channel overwrite create
	ae_channel_overwrite_create	=	13,
	/// Channel overwrite update
	ae_channel_overwrite_update	=	14,
	/// Channel overwrite delete
	ae_channel_overwrite_delete	=	15,
	/// Channel member kick
	ae_member_kick			=	20,
	/// Channel member prune
	ae_member_prune			=	21,
	/// Channel member ban add
	ae_member_ban_add		=	22,
	/// Channel member ban remove
	ae_member_ban_remove		=	23,
	/// Guild member update
	ae_member_update		=	24,
	/// Guild member role update
	ae_member_role_update		=	25,
	/// Guild member move
	ae_member_move			=	26,
	/// Guild member voice disconnect
	ae_member_disconnect		=	27,
	/// Guild bot add
	ae_bot_add			=	28,
	/// Guild role create
	ae_role_create			=	30,
	/// Guild role update
	ae_role_update			=	31,
	/// Guild role delete
	ae_role_delete			=	32,
	/// Guild invite create
	ae_invite_create		=	40,
	/// Guild invite update
	ae_invite_update		=	41,
	/// Guild invite delete
	ae_invite_delete		=	42,
	/// Guild webhook create
	ae_webhook_create		=	50,
	/// Guild webhook update
	ae_webhook_update		=	51,
	/// Guild webhook delete
	ae_webhook_delete		=	52,
	/// Guild emoji create
	ae_emoji_create			=	60,
	/// Guild emoji update
	ae_emoji_update			=	61,
	/// Guild emoji delete
	ae_emoji_delete			=	62,
	/// Guild message delete
	ae_message_delete		=	72,
	/// Guild message bulk delete
	ae_message_bulk_delete		=	73,
	/// Guild message pin
	ae_message_pin			=	74,
	/// Guild message unpin
	ae_message_unpin		=	75,
	/// Guild integration create
	ae_integration_create		=	80,
	/// Guild integration update
	ae_integration_update		=	81,
	/// Guild integration delete
	ae_integration_delete		=	82,
	/// Stage instance create
	ae_stage_instance_create	=	83,
	/// Stage instance update
	ae_stage_instance_update	=	84,
	/// stage instance delete
	ae_stage_instance_delete	=	85,
	/// Sticker create
	ae_sticker_create		=	90,
	/// Sticker update
	ae_sticker_update		=	91,
	/// Sticker delete
	ae_sticker_delete		=	92,
	/// Scheduled event creation
	ae_guild_scheduled_event_create	=	100,
	/// Scheduled event update
	ae_guild_scheduled_event_update	=	101,
	/// Scheduled event deletion
	ae_guild_scheduled_event_delete	=	102,
	/// Thread create
	ae_thread_create		=	110,
	/// Thread update
	ae_thread_update		=	111,
	/// Thread delete
	ae_thread_delete		=	112,
	/// Application command permissions update
	ae_appcommand_permission_update	=	121,
	/// Auto moderation rule creation
	ae_automod_rule_create		=	140,
	/// Auto moderation rule update
	ae_automod_rule_update		=	141,
	/// Auto moderation rule deletion
	ae_automod_rule_delete		=	142,
	/// Auto moderation block message
	ae_automod_block_message	=	143,
};

/**
 * @brief Defines audit log changes
 */
struct DPP_EXPORT audit_change {
	/// Optional: Serialised new value of the key
	std::string	new_value;
	/// Optional: Serialised old value of the key
	std::string	old_value;
	/// name of audit log change key
	std::string	key;
};

/**
 * @brief Extra information for an audit log entry
 */
struct DPP_EXPORT audit_extra {
	std::string 	delete_member_days;	//!< number of days after which inactive members were kicked
	std::string	members_removed;	//!< number of members removed by the prune
	snowflake	channel_id;		//!< channel in which the entities were targeted
	snowflake	message_id;		//!< id of the message that was targeted
	std::string	count;			//!< number of entities that were targeted
	snowflake	id;			//!< id of the overwritten entity
	std::string	type;			//!< type of overwritten entity - "0" for "role" or "1" for "member"
	std::string 	role_name;		//!< name of the role if type is "0" (not present if type is "1")
};

/**
 * @brief An individual audit log entry
 */
struct DPP_EXPORT audit_entry {
	snowflake			id;		//!< id of the entry
	snowflake			target_id;	//!< id of the affected entity (webhook, user, role, etc.) (may be empty)
	std::vector<audit_change>	changes;	//!< Optional: changes made to the target_id
	snowflake			user_id;	//!< the user who made the changes (may be empty)
	audit_type			event;		//!< type of action that occurred
	std::optional<audit_extra>	options;	//!< Optional: additional info for certain action types
	std::string			reason;		//!< Optional: the reason for the change (0-512 characters)	
};

/**
 * @brief The auditlog class represents the audit log entry of a guild.
 */
class DPP_EXPORT auditlog : public json_interface<auditlog>  {
public:
	std::vector<audit_entry> entries;	//!< Audit log entries
	
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
