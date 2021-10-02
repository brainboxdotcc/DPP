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
#include <dpp/discord.h>
#include <dpp/message.h>
#include <dpp/slashcommand.h>
#include <functional>
#include <variant>
#include <exception>

namespace dpp {

struct CoreExport confirmation_callback_t;

typedef std::function<void(const confirmation_callback_t&)> command_completion_event_t;

/** @brief Base event parameter struct */
struct CoreExport event_dispatch_t {

	/** Raw event text */
	std::string raw_event;

	/** Shard the event came from */
	class discord_client* from; 
	
	/** Constructor
 	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	event_dispatch_t(class discord_client* client, const std::string& raw);
};

/** @brief Log messages */
struct CoreExport log_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on. CAN BE NULL
	 * for log events originating from the cluster object
	 * @param raw Raw event text as JSON
	 */
	log_t(class discord_client* client, const std::string& raw);
	/** Severity */
	loglevel severity;
	/** Log Message */
	std::string message;
};

/** @brief Create stage instance */
struct CoreExport stage_instance_create_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on. CAN BE NULL
	 * for log events originating from the cluster object
	 * @param raw Raw event text as JSON
	 */
	stage_instance_create_t(class discord_client* client, const std::string& raw);
	/**
	 * @brief stage instance id
	 */
	snowflake id;
	/**
	 * @brief Channel ID
	 */
	snowflake channel_id;
	/**
	 * @brief Guild ID
	 */
	snowflake guild_id;
	/**
	 * @brief Privacy level
	 */
	uint8_t privacy_level;
	/**
	 * @brief Stage Topic
	 */
	std::string topic;
};

/** @brief Delete stage instance */
struct CoreExport stage_instance_delete_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on. CAN BE NULL
	 * for log events originating from the cluster object
	 * @param raw Raw event text as JSON
	 */
	stage_instance_delete_t(class discord_client* client, const std::string& raw);
	/**
	 * @brief stage instance id
	 */
	snowflake id;
	/**
	 * @brief Channel ID
	 */
	snowflake channel_id;
	/**
	 * @brief Guild ID
	 */
	snowflake guild_id;
	/**
	 * @brief Privacy level
	 */
	uint8_t privacy_level;
	/**
	 * @brief Stage Topic
	 */
	std::string topic;
};

/** @brief Voice state update */
struct CoreExport voice_state_update_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	voice_state_update_t(class discord_client* client, const std::string& raw);
	/** Voice state */
	voicestate state;
};

/**
 * @brief Create interaction
 */
struct CoreExport interaction_create_t : public event_dispatch_t {

	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	interaction_create_t(class discord_client* client, const std::string& raw);

	/**
	 * @brief Send a reply for this interaction
	 * 
	 * @param t Type of reply to send
	 * @param m Message object to send. Not all fields are supported by Discord.
	 */
	void reply(interaction_response_type t, const message & m) const;

	/**
	 * @brief Send a reply for this interaction
	 * 
	 * @param t Type of reply to send
	 * @param mt The string value to send, for simple text only messages
	 */
	void reply(interaction_response_type t, const std::string & mt) const;

	/**
	 * @brief Get original response message for this interaction
	 *
	 * @param callback Function to call when the API call completes.
	 * On success the callback will contain a dpp::message object in confirmation_callback_t::value. On failure, the value is undefined and confirmation_callback_t::is_error() method will return true. You can obtain full error details with confirmation_callback_t::get_error().
	 */
	void get_original_response(command_completion_event_t callback = {}) const;

	/**
	 * @brief Edit the response for this interaction
	 *
	 * @param m Message object to send. Not all fields are supported by Discord.
	 */
	void edit_response(const message & m) const;

	/**
	 * @brief Edit the response for this interaction
	 *
	 * @param mt The string value to send, for simple text only messages
	 */
	void edit_response(const std::string & mt) const;

	/**
	 * @brief Get a command line parameter
	 * 
	 * @param name The command line parameter to retrieve
	 * @return const command_value& If the command line parameter does not 
	 * exist, an empty variant is returned.
	 */
	const virtual command_value& get_parameter(const std::string& name) const;

	interaction command;
};

/**
 * @brief Click on button
 */
struct CoreExport button_click_t : public interaction_create_t {

	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	button_click_t(class discord_client* client, const std::string& raw);

	/**
	 * @brief Get a command line parameter
	 * 
	 * @param name The command line parameter to retrieve
	 * @return Always returns an empty parameter as buttons dont have parameters!
	 */
	const virtual command_value& get_parameter(const std::string& name) const;

	std::string custom_id;
	uint8_t component_type;
};

/**
 * @brief Click on select
 */
struct CoreExport select_click_t : public interaction_create_t {

	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	select_click_t(class discord_client* client, const std::string& raw);

	/**
	 * @brief Get a command line parameter
	 * 
	 * @param name The command line parameter to retrieve
	 * @return Always returns an empty parameter as buttons dont have parameters!
	 */
	const virtual command_value& get_parameter(const std::string& name) const;

	std::string custom_id;
	std::vector<std::string> values;
	uint8_t component_type;
};


/** @brief Delete guild */
struct CoreExport guild_delete_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	guild_delete_t(class discord_client* client, const std::string& raw);
	/** Deleted guild */
	guild* deleted;
};

/** @brief Update guild stickers */
struct CoreExport guild_stickers_update_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	guild_stickers_update_t(class discord_client* client, const std::string& raw);
	/** Deleted guild */
	guild* updating_guild;
	std::vector<sticker> stickers;
};

/** @brief Guild join request delete (user declined membership screening) */
struct CoreExport guild_join_request_delete_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	guild_join_request_delete_t(class discord_client* client, const std::string& raw);
	/** Deleted guild */
	snowflake guild_id;
	snowflake user_id;
};

/** @brief Delete channel */
struct CoreExport channel_delete_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	channel_delete_t(class discord_client* client, const std::string& raw);
	guild* deleting_guild;
	channel* deleted;
};

/** @brief Update channel */
struct CoreExport channel_update_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	channel_update_t(class discord_client* client, const std::string& raw);
	guild* updating_guild;
	channel* updated;
};

/** @brief Session ready */
struct CoreExport ready_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	ready_t(class discord_client* client, const std::string& raw);
	std::string session_id;
	uint32_t shard_id;
};

/** @brief Message Deleted */
struct CoreExport message_delete_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	message_delete_t(class discord_client* client, const std::string& raw);
	message* deleted;
};

struct CoreExport application_command_delete_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	application_command_delete_t(class discord_client* client, const std::string& raw);
};

/** @brief Guild member remove */
struct CoreExport guild_member_remove_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	guild_member_remove_t(class discord_client* client, const std::string& raw);
	guild* removing_guild;
	user* removed;
};

/**
 * @brief Create application slash command
 * 
 */
struct CoreExport application_command_create_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	application_command_create_t(class discord_client* client, const std::string& raw);
};

/** @brief Session resumed */
struct CoreExport resumed_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	resumed_t(class discord_client* client, const std::string& raw);
	std::string session_id;
	uint32_t shard_id;
};

/** @brief Guild role create */
struct CoreExport guild_role_create_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	guild_role_create_t(class discord_client* client, const std::string& raw);
	guild* creating_guild;
	role* created;
};

/** @brief Typing start */
struct CoreExport typing_start_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	typing_start_t(class discord_client* client, const std::string& raw);
	guild* typing_guild;
	channel* typing_channel;
	user* typing_user;
	time_t timestamp;
};

/** @brief Voice state update */
struct CoreExport voice_track_marker_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on.
	 * Will always be null.
	 * @param raw Raw event text as JSON.
	 * Will always be empty.
	 */
	voice_track_marker_t(class discord_client* client, const std::string& raw);
	/** Voice client */
	class discord_voice_client* voice_client;
	/** Track metadata */
	std::string track_meta;
};


/** @brief Message reaction add */
struct CoreExport message_reaction_add_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	message_reaction_add_t(class discord_client* client, const std::string& raw);
	guild* reacting_guild;
	user reacting_user;
	guild_member reacting_member;
	channel* reacting_channel;
	emoji reacting_emoji;
	snowflake message_id;
};

/** @brief Guild members chunk */
struct CoreExport guild_members_chunk_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	guild_members_chunk_t(class discord_client* client, const std::string& raw);
	guild* adding;
	guild_member_map* members;
};

/** @brief Message reaction remove */
struct CoreExport message_reaction_remove_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	message_reaction_remove_t(class discord_client* client, const std::string& raw);
	guild* reacting_guild;
	user reacting_user;
	guild_member reacting_member;
	channel* reacting_channel;
	emoji reacting_emoji;
	snowflake message_id;
};

/** @brief Create guild */
struct CoreExport guild_create_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	guild_create_t(class discord_client* client, const std::string& raw);
	guild* created;
};

/** @brief Create channel */
struct CoreExport channel_create_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	channel_create_t(class discord_client* client, const std::string& raw);
	guild* creating_guild;
	channel* created;
};

/** @brief Message remove emoji */
struct CoreExport message_reaction_remove_emoji_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	message_reaction_remove_emoji_t(class discord_client* client, const std::string& raw);
	guild* reacting_guild;
	channel* reacting_channel;
	emoji reacting_emoji;
	snowflake message_id;
};

/** @brief Message delete bulk */
struct CoreExport message_delete_bulk_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	message_delete_bulk_t(class discord_client* client, const std::string& raw);
	guild* deleting_guild;
	user* deleting_user;
	channel* deleting_channel;
	std::vector<snowflake> deleted;
};

/** @brief Guild role update */
struct CoreExport guild_role_update_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	guild_role_update_t(class discord_client* client, const std::string& raw);
	guild* updating_guild;
	role* updated;
};

/** @brief Guild role delete */
struct CoreExport guild_role_delete_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	guild_role_delete_t(class discord_client* client, const std::string& raw);
	guild* deleting_guild;
	role* deleted;
};

/** @brief Channel pins update */
struct CoreExport channel_pins_update_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	channel_pins_update_t(class discord_client* client, const std::string& raw);
	guild* pin_guild;
	channel* pin_channel;
	time_t timestamp;
};

/** @brief Message remove all reactions */
struct CoreExport message_reaction_remove_all_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	message_reaction_remove_all_t(class discord_client* client, const std::string& raw);
        guild* reacting_guild;
        channel* reacting_channel;
        snowflake message_id;
};

/** @brief Voice server update */
struct CoreExport voice_server_update_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	voice_server_update_t(class discord_client* client, const std::string& raw);
	snowflake guild_id;
	std::string token;
	std::string endpoint;
};

/** @brief Guild emojis update */
struct CoreExport guild_emojis_update_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	guild_emojis_update_t(class discord_client* client, const std::string& raw);
	std::vector<snowflake> emojis;
	guild* updating_guild;
};

/**
 * @brief Presence update
 * 
 */
struct CoreExport presence_update_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	presence_update_t(class discord_client* client, const std::string& raw);
	presence rich_presence;
};

/** @brief Webhooks update */
struct CoreExport webhooks_update_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	webhooks_update_t(class discord_client* client, const std::string& raw);
	guild* webhook_guild;
	channel* webhook_channel;
};

/** @brief Guild member add */
struct CoreExport guild_member_add_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	guild_member_add_t(class discord_client* client, const std::string& raw);
	guild* adding_guild;
	guild_member added;
};

/** @brief Invite delete */
struct CoreExport invite_delete_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	invite_delete_t(class discord_client* client, const std::string& raw);
	invite deleted_invite;
};

/** @brief Guild update */
struct CoreExport guild_update_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	guild_update_t(class discord_client* client, const std::string& raw);
	guild* updated;
};

/** @brief Guild integrations update */
struct CoreExport guild_integrations_update_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	guild_integrations_update_t(class discord_client* client, const std::string& raw);
	guild* updating_guild;
};

/** @brief Guild member update */
struct CoreExport guild_member_update_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	guild_member_update_t(class discord_client* client, const std::string& raw);
	guild* updating_guild;
	guild_member updated;
};

/**
 * @brief Update application slash command
 * 
 */
struct CoreExport application_command_update_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	application_command_update_t(class discord_client* client, const std::string& raw);
};

/** @brief Invite create */
struct CoreExport invite_create_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	invite_create_t(class discord_client* client, const std::string& raw);
	invite created_invite;
};

/** @brief Message update */
struct CoreExport message_update_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	message_update_t(class discord_client* client, const std::string& raw);
	message* updated;
};

/* @brief User update */
struct CoreExport user_update_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	user_update_t(class discord_client* client, const std::string& raw);
	user updated;
};

/** @brief Create message */
struct CoreExport message_create_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	message_create_t(class discord_client* client, const std::string& raw);
	message* msg;
};

/** @brief Guild ban add */
struct CoreExport guild_ban_add_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	guild_ban_add_t(class discord_client* client, const std::string& raw);
	guild* banning_guild;
	user banned;
};

/** @brief Guild ban remove */
struct CoreExport guild_ban_remove_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	guild_ban_remove_t(class discord_client* client, const std::string& raw);
	guild* unbanning_guild;
	user unbanned;
};

/** @brief Integration create */
struct CoreExport integration_create_t : public event_dispatch_t {
	/** Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	integration_create_t(class discord_client* client, const std::string& raw);
	integration created_integration;
};

/** @brief Integration update */
struct CoreExport integration_update_t : public event_dispatch_t {
	/** 
	 * @brief Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	integration_update_t(class discord_client* client, const std::string& raw);
	integration updated_integration;
};

/** @brief Integration delete */
struct CoreExport integration_delete_t : public event_dispatch_t {
	/** 
	 * @brief Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	integration_delete_t(class discord_client* client, const std::string& raw);
	integration deleted_integration;
};

/** @brief Thread Create*/
struct CoreExport thread_create_t : public event_dispatch_t {
	/** 
	 * @brief Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	thread_create_t(class discord_client* client, const std::string& raw);
	guild* creating_guild;
	channel created;
};

/** @brief Thread Update
*/
struct CoreExport thread_update_t : public event_dispatch_t {
	/** 
	 * @brief Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	thread_update_t(class discord_client* client, const std::string& raw);
	guild* updating_guild;
	channel updated;
};

/** @brief Thread Delete
 */
struct CoreExport thread_delete_t : public event_dispatch_t {
	/** 
	 * @brief Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	thread_delete_t(class discord_client* client, const std::string& raw);
	guild* deleting_guild;
	channel deleted;
};

/** @brief Thread List Sync
 */
struct CoreExport thread_list_sync_t : public event_dispatch_t {
	/** 
	 * @brief Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	thread_list_sync_t(class discord_client* client, const std::string& raw);
	guild* updating_guild;
	std::vector<channel> threads;
	std::vector<thread_member> members;
};

/** @brief Thread Member Update
 */
struct CoreExport thread_member_update_t : public event_dispatch_t {
	/** 
	 * @brief Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	thread_member_update_t(class discord_client* client, const std::string& raw);
	thread_member updated;
};

/** @brief Thread Members Update
 */
struct CoreExport thread_members_update_t : public event_dispatch_t {
	/** 
	 * @brief Constructor
	 * @param client The shard the event originated on
	 * @param raw Raw event text as JSON
	 */
	thread_members_update_t(class discord_client* client, const std::string& raw);
	snowflake thread_id;
	guild* updating_guild;
	uint8_t member_count;
	std::vector<thread_member> added;
	std::vector<snowflake> removed_ids;
};

/** @brief voice buffer send
 */
struct CoreExport voice_buffer_send_t : public event_dispatch_t {
	/** 
	 * @brief Constructor
	 * @param client The shard the event originated on
	 * WILL ALWAYS be NULL.
	 * @param raw Raw event text as JSON
	 */
	voice_buffer_send_t(class discord_client* client, const std::string &raw);
	class discord_voice_client* voice_client;
	int buffer_size;
};

/** @brief voice user talking */
struct CoreExport voice_user_talking_t : public event_dispatch_t {
	/** 
	 * @brief Constructor
	 * @param client The shard the event originated on
	 * WILL ALWAYS be NULL.
	 * @param raw Raw event text as JSON
	 */
	voice_user_talking_t(class discord_client* client, const std::string &raw);
	class discord_voice_client* voice_client;
	snowflake user_id;
	uint8_t talking_flags;
};

/** @brief voice user talking */
struct CoreExport voice_ready_t : public event_dispatch_t {
	/** 
	 * @brief Constructor
	 * @param client The shard the event originated on
	 * WILL ALWAYS be NULL.
	 * @param raw Raw event text as JSON
	 */
	voice_ready_t(class discord_client* client, const std::string &raw);
	class discord_voice_client* voice_client;
	snowflake voice_channel_id;
};

/** @brief voice receive packet */
struct CoreExport voice_receive_t : public event_dispatch_t {
	/** 
	 * @brief Constructor
	 * @param client The shard the event originated on.
	 * WILL ALWAYS be NULL.
	 * @param raw Raw event text as JSON
	 */
	voice_receive_t(class discord_client* client, const std::string &raw);
	class discord_voice_client* voice_client;
	/**
	 * @brief Audio data, encoded as 48kHz stereo PCM or Opus
	 */
	uint8_t* audio;
	/**
	 * @brief Size of audio buffer
	 */
	size_t audio_size;
	/**
	 * @brief User ID of speaker (zero if unknown)
	 */
	snowflake user_id;
};

/** @brief voice client speaking event */
struct CoreExport voice_client_speaking_t : public event_dispatch_t {
	/** 
	 * @brief Constructor
	 * @param client The shard the event originated on.
	 * WILL ALWAYS be NULL.
	 * @param raw Raw event text as JSON
	 */
	voice_client_speaking_t(class discord_client* client, const std::string &raw);
	class discord_voice_client* voice_client;
	snowflake user_id;
	uint32_t ssrc;
};

/** @brief voice client disconnect event */
struct CoreExport voice_client_disconnect_t : public event_dispatch_t {
	/** 
	 * @brief Constructor
	 * @param client The shard the event originated on.
	 * WILL ALWAYS be NULL.
	 * @param raw Raw event text as JSON
	 */
	voice_client_disconnect_t(class discord_client* client, const std::string &raw);
	class discord_voice_client* voice_client;
	snowflake user_id;
};

/** @brief The dispatcher class contains a set of std::functions representing hooked events
 * that the user code is interested in. These are modified via the on_eventname style
 * methods in the cluster class.
 */
class CoreExport dispatcher {
public:
	/** @brief Event handler function pointer for log event
	 * @param event Event parameters
	 */
	std::function<void(const log_t& event)> log;
	/** @brief Event handler function pointer for voice state update event
	 * @param event Event parameters
	 */
	std::function<void(const voice_state_update_t& event)> voice_state_update;
	/** @brief Event handler function pointer for voice client speaking event
	 * @param event Event parameters
	 */
	std::function<void(const voice_client_speaking_t& event)> voice_client_speaking;
	/** @brief Event handler function pointer for voice client disconnect event
	 * @param event Event parameters
	 */
	std::function<void(const voice_client_disconnect_t& event)> voice_client_disconnect;
	/** @brief Event handler function pointer for interaction create event
	 * @param event Event parameters
	 */
	std::function<void(const interaction_create_t& event)> interaction_create;
	/** @brief Event handler function pointer for button click event
	 * @param event Event parameters
	 */
	std::function<void(const button_click_t& event)> button_click;
	/** @brief Event handler function pointer for button click event
	 * @param event Event parameters
	 */
	std::function<void(const select_click_t& event)> select_click;
	/** @brief Event handler function pointer for guild delete event
	 * @param event Event parameters
	 */
	std::function<void(const guild_delete_t& event)> guild_delete;
	/** @brief Event handler function pointer for channel delete event
	 * @param event Event parameters
	 */
	std::function<void(const channel_delete_t& event)> channel_delete;
	/** @brief Event handler function pointer for channel update event
	 * @param event Event parameters
	 */
	std::function<void(const channel_update_t& event)> channel_update;
	/** @brief Event handler function pointer for ready event
	 * @param event Event parameters
	 */
	std::function<void(const ready_t& event)> ready;
	/** @brief Event handler function pointer for message delete event
	 * @param event Event parameters
	 */
	std::function<void(const message_delete_t& event)> message_delete;
	/** @brief Event handler function pointer for application command delete event
	 * @param event Event parameters
	 */
	std::function<void(const application_command_delete_t& event)> application_command_delete;
	/** @brief Event handler function pointer for guild member remove event
	 * @param event Event parameters
	 */
	std::function<void(const guild_member_remove_t& event)> guild_member_remove;
	/** @brief Event handler function pointer for guild member remove event
	 * @param event Event parameters
	 */
	std::function<void(const application_command_create_t& event)> application_command_create;
	/** @brief Event handler function pointer for resumed event
	 * @param event Event parameters
	 */
	std::function<void(const resumed_t& event)> resumed;
	/** @brief Event handler function pointer for guild role create event
	 * @param event Event parameters
	 */
	std::function<void(const guild_role_create_t& event)> guild_role_create;
	/** @brief Event handler function pointer for typing start event
	 * @param event Event parameters
	 */
	std::function<void(const typing_start_t& event)> typing_start;
	/** @brief Event handler function pointer for message reaction add event
	 * @param event Event parameters
	 */
	std::function<void(const message_reaction_add_t& event)> message_reaction_add;
	/** @brief Event handler function pointer for guild members chunk event
	 * @param event Event parameters
	 */
	std::function<void(const guild_members_chunk_t& event)> guild_members_chunk;
	/** @brief Event handler function pointer for message reaction remove event
	 * @param event Event parameters
	 */
	std::function<void(const message_reaction_remove_t& event)> message_reaction_remove;
	/** @brief Event handler function pointer for guild create event
	 * @param event Event parameters
	 */
	std::function<void(const guild_create_t& event)> guild_create;
	/** @brief Event handler function pointer for guild channel create event
	 * @param event Event parameters
	 */
	std::function<void(const channel_create_t& event)> channel_create;
	/** @brief Event handler function pointer for message reaction remove emoji event
	 * @param event Event parameters
	 */
	std::function<void(const message_reaction_remove_emoji_t& event)> message_reaction_remove_emoji;
	/** @brief Event handler function pointer for message delete bulk event
	 * @param event Event parameters
	 */
	std::function<void(const message_delete_bulk_t& event)> message_delete_bulk;
	/** @brief Event handler function pointer for guild role update event
	 * @param event Event parameters
	 */
	std::function<void(const guild_role_update_t& event)> guild_role_update;
	/** @brief Event handler function pointer for guild role delete event
	 * @param event Event parameters
	 */
	std::function<void(const guild_role_delete_t& event)> guild_role_delete;
	/** @brief Event handler function pointer for channel pins update event
	 * @param event Event parameters
	 */
	std::function<void(const channel_pins_update_t& event)> channel_pins_update;
	/** @brief Event handler function pointer for message reaction remove all event
	 * @param event Event parameters
	 */
	std::function<void(const message_reaction_remove_all_t& event)> message_reaction_remove_all;
	/** @brief Event handler function pointer for voice server update event
	 * @param event Event parameters
	 */
	std::function<void(const voice_server_update_t& event)> voice_server_update;
	/** @brief Event handler function pointer for guild emojis update event
	 * @param event Event parameters
	 */
	std::function<void(const guild_emojis_update_t& event)> guild_emojis_update;
	/** @brief Event handler function pointer for presence update event
	 * @param event Event parameters
	 */
	std::function<void(const presence_update_t& event)> presence_update;
	/** @brief Event handler function pointer for webhooks update event
	 * @param event Event parameters
	 */
	std::function<void(const webhooks_update_t& event)> webhooks_update;
	/** @brief Event handler function pointer for guild member add event
	 * @param event Event parameters
	 */
	std::function<void(const guild_member_add_t& event)> guild_member_add;
	/** @brief Event handler function pointer for invite delete event
	 * @param event Event parameters
	 */
	std::function<void(const invite_delete_t& event)> invite_delete;
	/** @brief Event handler function pointer for guild update event
	 * @param event Event parameters
	 */
	std::function<void(const guild_update_t& event)> guild_update;
	/** @brief Event handler function pointer for guild integrations update event
	 * @param event Event parameters
	 */
	std::function<void(const guild_integrations_update_t& event)> guild_integrations_update;
	/** @brief Event handler function pointer for guild member update event
	 * @param event Event parameters
	 */
	std::function<void(const guild_member_update_t& event)> guild_member_update;
	/** @brief Event handler function pointer for application command update event
	 * @param event Event parameters
	 */
	std::function<void(const application_command_update_t& event)> application_command_update;
	/** @brief Event handler function pointer for invite create event
	 * @param event Event parameters
	 */
	std::function<void(const invite_create_t& event)> invite_create;
	/** @brief Event handler function pointer for message update event
	 * @param event Event parameters
	 */
	std::function<void(const message_update_t& event)> message_update;
	/** @brief Event handler function pointer for user update event
	 * @param event Event parameters
	 */
	std::function<void(const user_update_t& event)> user_update;
	/** @brief Event handler function pointer for message create event
	 * @param event Event parameters
	 */
	std::function<void(const message_create_t& event)> message_create;
	/** @brief Event handler function pointer for guild ban add event
	 * @param event Event parameters
	 */
	std::function<void(const guild_ban_add_t& event)> guild_ban_add;
	/** @brief Event handler function pointer for guild ban remove event
	 * @param event Event parameters
	 */
	std::function<void(const guild_ban_remove_t& event)> guild_ban_remove;
	/** @brief Event handler function pointer for integration create event
	 * @param event Event parameters
	 */
	std::function<void(const integration_create_t& event)> integration_create;
	/** @brief Event handler function pointer for integration update event
	 * @param event Event parameters
	 */
	std::function<void(const integration_update_t& event)> integration_update;
	/** @brief Event handler function pointer for integration delete event
	 * @param event Event parameters
	 */
	std::function<void(const integration_delete_t& event)> integration_delete;	
	/** @brief Event handler function pointer for thread create event 
	 * @param event Event parameters
	 */
	std::function<void(const thread_create_t& event)> thread_create;
	/** @brief Event handler function pointer for thread update event 
	 * @param event Event parameters
	 */
	std::function<void(const thread_update_t& event)> thread_update;
	/** @brief Event handler function pointer for thread delete event 
	 * @param event Event parameters
	 */
	std::function<void(const thread_delete_t& event)> thread_delete;
	/** @brief Event handler function pointer for thread list sync event
	 * @param event Event parameters
	 */
	std::function<void(const thread_list_sync_t& event)> thread_list_sync;
	/** @brief Event handler function pointer for thread member update event
	 * @param event Event parameters
	 */
	std::function<void(const thread_member_update_t& event)> thread_member_update;
	/** @brief Event handler function pointer for thread members update event
	 * @param event Event parameters
	 */
	std::function<void(const thread_members_update_t& event)> thread_members_update;
	/** @brief Event handler function pointer for voice buffer send event
	 * @param event Event parameters
	 */
	std::function<void(const voice_buffer_send_t& event)> voice_buffer_send;
	/** @brief Event handler function pointer for voice user talking event
	 * @param event Event parameters
	 */
	std::function<void(const voice_user_talking_t& event)> voice_user_talking;
	/** @brief Event handler function pointer for voice ready event
	 * @param event Event parameters
	 */
	std::function<void(const voice_ready_t& event)> voice_ready;
	/** @brief Event handler function pointer for voice receive event
	 * @param event Event parameters
	 */
	std::function<void(const voice_receive_t& event)> voice_receive;
	/** @brief Event handler function pointer for voice track marker event
	 * @param event Event parameters
	 */
	std::function<void(const voice_track_marker_t& event)> voice_track_marker;
	/** @brief Event handler function pointer for guild join request delete event
	 * @param event Event parameters
	 */
	std::function<void(const guild_join_request_delete_t& event)> guild_join_request_delete;
	/** @brief Event handler function pointer for stage instance create event
	 * @param event Event parameters
	 */
	std::function<void(const stage_instance_create_t& event)> stage_instance_create;
	/** @brief Event handler function pointer for stage instance delete event
	 * @param event Event parameters
	 */
	std::function<void(const stage_instance_delete_t& event)> stage_instance_delete;
	/** @brief Event handler function pointer for guild sticker update event
	 * @param event Event parameters
	 */
	std::function<void(const guild_stickers_update_t& event)> stickers_update;
};

/**
 * @brief The dpp::exception class derives from std::exception and supports some other
 * ways of passing in error details such as via std::string.
 */
class exception : public std::exception
{
	/**
	 * @brief Exception message
	 */
	std::string msg;

public:

	using std::exception::exception;

	/**
	 * @brief Construct a new exception object
	 */
	exception() = default;

	explicit exception(const char* what) : msg(what) { }
	exception(const char* what, size_t len) : msg(what, len) { }

	explicit exception(const std::string& what) : msg(what) { }
	explicit exception(std::string&& what) : msg(std::move(what)) { }

	/**
	 * @brief Construct a new exception object (copy constructor)
	 */
	exception(const exception&) = default;

	/**
	 * @brief Construct a new exception object (move constructor)
	 */
	exception(exception&&) = default;

	/**
	 * @brief Destroy the exception object
	 */
	~exception() override = default;

	/**
	 * @brief Copy assignment operator
	 * 
	 * @return exception& reference to self
	 */
	exception & operator = (const exception &) = default;

	/**
	 * @brief Move assignment operator
	 * 
	 * @return exception& reference to self
	 */
	exception & operator = (exception&&) = default;

	/**
	 * @brief Get exception message
	 * 
	 * @return const char* error message
	 */
	[[nodiscard]] const char* what() const noexcept override { return msg.c_str(); };

};

};

