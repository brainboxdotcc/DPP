#pragma once

#include <dpp/discord.h>
#include <dpp/message.h>
#include <functional>

namespace dpp {

/** Base event parameter struct */
struct event_dispatch_t {
	/** Raw event text */
	std::string raw_event;
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	event_dispatch_t(const std::string& raw);
};

/* Log messages */
struct log_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	log_t(const std::string& raw);
	/** Severity */
	loglevel severity;
	/** Log Message */
	std::string message;
};

/** Voice state update */
struct voice_state_update_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	voice_state_update_t(const std::string& raw);
	/** Voice state */
	voicestate state;
};

/**
 * @brief Create interaction
 */
struct interaction_create_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	interaction_create_t(const std::string& raw);
};

/** Delete guild */
struct guild_delete_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	guild_delete_t(const std::string& raw);
	/** Deleted guild */
	guild* deleted;
};

/** Delete channel */
struct channel_delete_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	channel_delete_t(const std::string& raw);
	guild* deleting_guild;
	channel* deleted;
};

/** Update channel */
struct channel_update_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	channel_update_t(const std::string& raw);
	guild* updating_guild;
	channel* updated;
};

/** Session ready */
struct ready_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	ready_t(const std::string& raw);
	std::string session_id;
	uint32_t shard_id;
};

/** Message Deleted */
struct message_delete_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	message_delete_t(const std::string& raw);
	message* deleted;
};

struct application_command_delete_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	application_command_delete_t(const std::string& raw);
};

/** Guild member remove */
struct guild_member_remove_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	guild_member_remove_t(const std::string& raw);
	guild* removing_guild;
	user* removed;
};

struct application_command_create_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	application_command_create_t(const std::string& raw);
};

/** Session resumed */
struct resumed_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	resumed_t(const std::string& raw);
	std::string session_id;
	uint32_t shard_id;
};

/** Guild role create */
struct guild_role_create_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	guild_role_create_t(const std::string& raw);
	guild* creating_guild;
	role* created;
};

/** Typing start */
struct typing_start_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	typing_start_t(const std::string& raw);
	guild* typing_guild;
	channel* typing_channel;
	user* typing_user;
	time_t timestamp;
};

/** Message reaction add */
struct message_reaction_add_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	message_reaction_add_t(const std::string& raw);
	guild* reacting_guild;
	user* reacting_user;
	channel* reacting_channel;
	emoji* reacting_emoji;
	snowflake message_id;
};

/** Guild members chunk */
struct guild_members_chunk_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	guild_members_chunk_t(const std::string& raw);
	guild* adding;
	guild_member_map* members;
};

/** Message reaction remove */
struct message_reaction_remove_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	message_reaction_remove_t(const std::string& raw);
        guild* reacting_guild;
        user* reacting_user;
        channel* reacting_channel;
        emoji* reacting_emoji;
        snowflake message_id;
};

/** Create guild */
struct guild_create_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	guild_create_t(const std::string& raw);
	guild* created;
};

/** Create channel */
struct channel_create_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	channel_create_t(const std::string& raw);
	guild* creating_guild;
	channel* created;
};

/** Message remove emoji */
struct message_reaction_remove_emoji_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	message_reaction_remove_emoji_t(const std::string& raw);
        guild* reacting_guild;
        channel* reacting_channel;
        emoji* reacting_emoji;
        snowflake message_id;
};

/** Message delete bulk */
struct message_delete_bulk_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	message_delete_bulk_t(const std::string& raw);
	guild* deleting_guild;
	user* deleting_user;
	channel* deleting_channel;
	std::vector<snowflake> deleted;
};

/** Guild role update */
struct guild_role_update_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	guild_role_update_t(const std::string& raw);
	guild* updating_guild;
	role* updated;
};

/** Guild role delete */
struct guild_role_delete_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	guild_role_delete_t(const std::string& raw);
	guild* deleting_guild;
	role* deleted;
};

/** Channel pins update */
struct channel_pins_update_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	channel_pins_update_t(const std::string& raw);
	guild* pin_guild;
	channel* pin_channel;
	time_t timestamp;
};

/** Message remove all reactions */
struct message_reaction_remove_all_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	message_reaction_remove_all_t(const std::string& raw);
        guild* reacting_guild;
        channel* reacting_channel;
        snowflake message_id;
};

/** Voice server update */
struct voice_server_update_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	voice_server_update_t(const std::string& raw);
	snowflake guild_id;
	std::string token;
	std::string endpoint;
};

/** Guild emojis update */
struct guild_emojis_update_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	guild_emojis_update_t(const std::string& raw);
	std::vector<snowflake> emojis;
	guild* updating_guild;
};

struct presence_update_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	presence_update_t(const std::string& raw);
	presence rich_presence;
};

/** Webhooks update */
struct webhooks_update_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	webhooks_update_t(const std::string& raw);
	guild* webhook_guild;
	channel* webhook_channel;
};

/** Guild member add */
struct guild_member_add_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	guild_member_add_t(const std::string& raw);
	guild* adding_guild;
	guild_member* added;
};

/** Invite delete */
struct invite_delete_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	invite_delete_t(const std::string& raw);
	invite deleted_invite;
};

/** Guild update */
struct guild_update_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	guild_update_t(const std::string& raw);
	guild* updated;
};

/** Guild integrations update */
struct guild_integrations_update_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	guild_integrations_update_t(const std::string& raw);
	guild* updating_guild;
};

/** Guild member update */
struct guild_member_update_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	guild_member_update_t(const std::string& raw);
	guild* updating_guild;
	guild_member* updated;
};

struct application_command_update_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	application_command_update_t(const std::string& raw);
};

/** Invite create */
struct invite_create_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	invite_create_t(const std::string& raw);
	invite created_invite;
};

/** Message update */
struct message_update_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	message_update_t(const std::string& raw);
	message* updated;
};

/* User update */
struct user_update_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	user_update_t(const std::string& raw);
	user* updated;
};

/** Create message */
struct message_create_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	message_create_t(const std::string& raw);
	message* msg;
};

/** Guild ban add */
struct guild_ban_add_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	guild_ban_add_t(const std::string& raw);
	guild* banning_guild;
	user banned;
};

/** Guild ban remove */
struct guild_ban_remove_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	guild_ban_remove_t(const std::string& raw);
	guild* unbanning_guild;
	user unbanned;
};

/** Integration create */
struct integration_create_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	integration_create_t(const std::string& raw);
	integration created_integration;
};

/** Integration update */
struct integration_update_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	integration_update_t(const std::string& raw);
	integration updated_integration;
};

/** Integration delete */
struct integration_delete_t : public event_dispatch_t {
	/** Constructor
	 * @param raw Raw event text as JSON
	 */
	integration_delete_t(const std::string& raw);
	integration deleted_integration;
};

/** The dispatcher class contains a set of std::functions representing hooked events
 * that the user code is interested in. These are modified via the on_eventname style
 * methods in the cluster class.
 */
class dispatcher {
public:
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const log_t& event)> log;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const voice_state_update_t& event)> voice_state_update;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const interaction_create_t& event)> interaction_create;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const guild_delete_t& event)> guild_delete;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const channel_delete_t& event)> channel_delete;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const channel_update_t& event)> channel_update;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const ready_t& event)> ready;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const message_delete_t& event)> message_delete;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const application_command_delete_t& event)> application_command_delete;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const guild_member_remove_t& event)> guild_member_remove;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const application_command_create_t& event)> application_command_create;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const resumed_t& event)> resumed;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const guild_role_create_t& event)> guild_role_create;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const typing_start_t& event)> typing_start;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const message_reaction_add_t& event)> message_reaction_add;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const guild_members_chunk_t& event)> guild_members_chunk;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const message_reaction_remove_t& event)> message_reaction_remove;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const guild_create_t& event)> guild_create;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const channel_create_t& event)> channel_create;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const message_reaction_remove_emoji_t& event)> message_reaction_remove_emoji;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const message_delete_bulk_t& event)> message_delete_bulk;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const guild_role_update_t& event)> guild_role_update;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const guild_role_delete_t& event)> guild_role_delete;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const channel_pins_update_t& event)> channel_pins_update;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const message_reaction_remove_all_t& event)> message_reaction_remove_all;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const voice_server_update_t& event)> voice_server_update;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const guild_emojis_update_t& event)> guild_emojis_update;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const presence_update_t& event)> presence_update;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const webhooks_update_t& event)> webhooks_update;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const guild_member_add_t& event)> guild_member_add;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const invite_delete_t& event)> invite_delete;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const guild_update_t& event)> guild_update;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const guild_integrations_update_t& event)> guild_integrations_update;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const guild_member_update_t& event)> guild_member_update;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const application_command_update_t& event)> application_command_update;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const invite_create_t& event)> invite_create;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const message_update_t& event)> message_update;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const user_update_t& event)> user_update;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const message_create_t& event)> message_create;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const guild_ban_add_t& event)> guild_ban_add;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const guild_ban_remove_t& event)> guild_ban_remove;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const integration_create_t& event)> integration_create;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const integration_update_t& event)> integration_update;
	/** Event handler function pointer
	 * @param event Event parameters
	 */
	std::function<void(const integration_delete_t& event)> integration_delete;
};

};

