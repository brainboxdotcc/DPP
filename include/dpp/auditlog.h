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
#include <dpp/json_fwd.h>
#include <optional>
#include <dpp/json_interface.h>

namespace dpp {

/**
 * @brief Defines types of audit log entry
 */
enum audit_type {
	/// Guild update
	aut_guild_update			=	1,
	/// Channel create
	aut_channel_create		=	10,
	/// Channel update
	aut_channel_update		=	11,
	/// Channel delete
	aut_channel_delete		=	12,
	/// Channel overwrite create
	aut_channel_overwrite_create	=	13,
	/// Channel overwrite update
	aut_channel_overwrite_update	=	14,
	/// Channel overwrite delete
	aut_channel_overwrite_delete	=	15,
	/// Channel member kick
	aut_member_kick			=	20,
	/// Channel member prune
	aut_member_prune			=	21,
	/// Channel member ban add
	aut_member_ban_add		=	22,
	/// Channel member ban remove
	aut_member_ban_remove		=	23,
	/// Guild member update
	aut_member_update		=	24,
	/// Guild member role update
	aut_member_role_update		=	25,
	/// Guild member move
	aut_member_move			=	26,
	/// Guild member voice disconnect
	aut_member_disconnect		=	27,
	/// Guild bot add
	aut_bot_add			=	28,
	/// Guild role create
	aut_role_create			=	30,
	/// Guild role update
	aut_role_update			=	31,
	/// Guild role delete
	aut_role_delete			=	32,
	/// Guild invite create
	aut_invite_create		=	40,
	/// Guild invite update
	aut_invite_update		=	41,
	/// Guild invite delete
	aut_invite_delete		=	42,
	/// Guild webhook create
	aut_webhook_create		=	50,
	/// Guild webhook update
	aut_webhook_update		=	51,
	/// Guild webhook delete
	aut_webhook_delete		=	52,
	/// Guild emoji create
	aut_emoji_create			=	60,
	/// Guild emoji update
	aut_emoji_update			=	61,
	/// Guild emoji delete
	aut_emoji_delete			=	62,
	/// Guild message delete
	aut_message_delete		=	72,
	/// Guild message bulk delete
	aut_message_bulk_delete		=	73,
	/// Guild message pin
	aut_message_pin			=	74,
	/// Guild message unpin
	aut_message_unpin		=	75,
	/// Guild integration create
	aut_integration_create		=	80,
	/// Guild integration update
	aut_integration_update		=	81,
	/// Guild integration delete
	aut_integration_delete		=	82,
	/// Stage instance create
	aut_stage_instance_create	=	83,
	/// Stage instance update
	aut_stage_instance_update	=	84,
	/// stage instance delete
	aut_stage_instance_delete	=	85,
	/// Sticker create
	aut_sticker_create		=	90,
	/// Sticker update
	aut_sticker_update		=	91,
	/// Sticker delete
	aut_sticker_delete		=	92,
	/// Scheduled event creation
	aut_guild_scheduled_event_create	=	100,
	/// Scheduled event update
	aut_guild_scheduled_event_update	=	101,
	/// Scheduled event deletion
	aut_guild_scheduled_event_delete	=	102,
	/// Thread create
	aut_thread_create		=	110,
	/// Thread update
	aut_thread_update		=	111,
	/// Thread delete
	aut_thread_delete		=	112,
	/// Application command permissions update
	aut_appcommand_permission_update	=	121,
	/// Auto moderation rule creation
	aut_automod_rule_create		=	140,
	/// Auto moderation rule update
	aut_automod_rule_update		=	141,
	/// Auto moderation rule deletion
	aut_automod_rule_delete		=	142,
	/// Message was blocked by Auto Moderation
	aut_automod_block_message	=	143,
	/// Message was flagged by Auto Moderation
	aut_automod_flag_to_channel =	144,
	/// Member was timed out by Auto Moderation
	aut_automod_user_communication_disabled =	145,
	/// Creator monetization request was created
	aut_creator_monetization_request_created = 150,
	/// Creator monetization terms were accepted
	aut_creator_monetization_terms_accepted = 151,
};

/**
 * @brief Defines audit log changes
 */
struct DPP_EXPORT audit_change {
	/// Optional: Serialised new value of the change, e.g. for nicknames, the new nickname
	std::string	new_value;
	/// Optional: Serialised old value of the change, e.g. for nicknames, the old nickname
	std::string	old_value;
	/**
	 * The property name that was changed, e.g. `nick` for nickname changes
	 * @note For dpp::aut_appcommand_permission_update updates the key is the id of the user, channel, role, or a permission constant that was updated instead of an actual property name
	 */
	std::string	key;
};

/**
 * @brief Extra information for an audit log entry
 */
struct DPP_EXPORT audit_extra {
	std::string automod_rule_name; //!< Name of the Auto Moderation rule that was triggered
	std::string automod_rule_trigger_type; //!< Trigger type of the Auto Moderation rule that was triggered
	std::string delete_member_days;	//!< number of days after which inactive members were kicked
	std::string	members_removed;	//!< number of members removed by the prune
	snowflake	channel_id;		//!< channel in which the entities were targeted
	snowflake	message_id;		//!< id of the message that was targeted
	std::string	count;			//!< number of entities that were targeted
	snowflake	id;			//!< id of the overwritten entity
	std::string	type;			//!< type of overwritten entity - "0" for "role" or "1" for "member"
	std::string role_name;		//!< name of the role if type is "0" (not present if type is "1")
	snowflake	application_id;	//!< ID of the app whose permissions were targeted
};

/**
 * @brief An individual audit log entry
 */
struct DPP_EXPORT audit_entry : public json_interface<audit_entry> {
	snowflake			id;		//!< id of the entry
	/**
	 * ID of the affected entity (webhook, user, role, etc.) (may be empty)
	 * @note For dpp::audit_type::aut_appcommand_permission_update updates, it's the command ID or the app ID
	 */
	snowflake			target_id;
	std::vector<audit_change>	changes;	//!< Optional: changes made to the target_id
	snowflake			user_id;	//!< the user or app that made the changes (may be empty)
	audit_type			type;		//!< type of action that occurred
	std::optional<audit_extra>	extra;	//!< Optional: additional info for certain action types
	std::string			reason;		//!< Optional: the reason for the change (1-512 characters)

	/** Constructor */
	audit_entry();

	/** Destructor */
	virtual ~audit_entry() = default;

	/** Read class values from json object
	 * @param j A json object to read from
	 * @return A reference to self
	 */
	audit_entry& fill_from_json(nlohmann::json* j);
};

/**
 * @brief The auditlog class represents the audit log entries of a guild.
 */
class DPP_EXPORT auditlog : public json_interface<auditlog>  {
public:
	std::vector<audit_entry> entries;	//!< Audit log entries
	
	/** Constructor */
	auditlog() = default;

	/** Destructor */
	virtual ~auditlog() = default;

	/** Read class values from json object
	 * @param j A json object to read from
	 * @return A reference to self
	 */
	 auditlog& fill_from_json(nlohmann::json* j);
};

};
