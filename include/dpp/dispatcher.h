#pragma once

#include <dpp/discord.h>
#include <dpp/message.h>
#include <functional>

namespace dpp {

struct event_dispatch_t {
	std::string raw_event;
	event_dispatch_t(const std::string& raw);
};

struct voice_state_update_t : public event_dispatch_t {
	voice_state_update_t(const std::string& raw);
};

struct interaction_create_t : public event_dispatch_t {
	interaction_create_t(const std::string& raw);
};

/** Delete guild */
struct guild_delete_t : public event_dispatch_t {
	guild_delete_t(const std::string& raw);
	guild* deleted;
};

/** Delete channel */
struct channel_delete_t : public event_dispatch_t {
	channel_delete_t(const std::string& raw);
	guild* deleting_guild;
	channel* deleted;
};

/** Update channel */
struct channel_update_t : public event_dispatch_t {
	channel_update_t(const std::string& raw);
	guild* updating_guild;
	channel* updated;
};

/** Session ready */
struct ready_t : public event_dispatch_t {
	ready_t(const std::string& raw);
	std::string session_id;
	uint32_t shard_id;
};

/* Message Deleted */
struct message_delete_t : public event_dispatch_t {
	message_delete_t(const std::string& raw);
	message* deleted;
};

struct application_command_delete_t : public event_dispatch_t {
	application_command_delete_t(const std::string& raw);
};

/* Guild member remove */
struct guild_member_remove_t : public event_dispatch_t {
	guild_member_remove_t(const std::string& raw);
	guild* removing_guild;
	user* removed;
};

struct application_command_create_t : public event_dispatch_t {
	application_command_create_t(const std::string& raw);
};

/** Session resumed */
struct resumed_t : public event_dispatch_t {
	resumed_t(const std::string& raw);
	std::string session_id;
	uint32_t shard_id;
};

/* Guild role create */
struct guild_role_create_t : public event_dispatch_t {
	guild_role_create_t(const std::string& raw);
	guild* creating_guild;
	role* created;
};

/** Typing start */
struct typing_start_t : public event_dispatch_t {
	typing_start_t(const std::string& raw);
	guild* typing_guild;
	channel* typing_channel;
	user* typing_user;
	time_t timestamp;
};

/* Message reaction add */
struct message_reaction_add_t : public event_dispatch_t {
	message_reaction_add_t(const std::string& raw);
	guild* reacting_guild;
	user* reacting_user;
	channel* reacting_channel;
	emoji* reacting_emoji;
	snowflake message_id;
};

struct guild_members_chunk_t : public event_dispatch_t {
	guild_members_chunk_t(const std::string& raw);
	guild* adding;
	guild_member_map* members;
};

/* Message reaction remove */
struct message_reaction_remove_t : public event_dispatch_t {
	message_reaction_remove_t(const std::string& raw);
        guild* reacting_guild;
        user* reacting_user;
        channel* reacting_channel;
        emoji* reacting_emoji;
        snowflake message_id;
};

/** Create guild */
struct guild_create_t : public event_dispatch_t {
	guild_create_t(const std::string& raw);
	guild* created;
};

/** Create channel */
struct channel_create_t : public event_dispatch_t {
	channel_create_t(const std::string& raw);
	guild* creating_guild;
	channel* created;
};

/* Message remove emoji */
struct message_reaction_remove_emoji_t : public event_dispatch_t {
	message_reaction_remove_emoji_t(const std::string& raw);
        guild* reacting_guild;
        channel* reacting_channel;
        emoji* reacting_emoji;
        snowflake message_id;
};

struct message_delete_bulk_t : public event_dispatch_t {
	message_delete_bulk_t(const std::string& raw);
	guild* deleting_guild;
	user* deleting_user;
	channel* deleting_channel;
	std::vector<snowflake> deleted;
};

/* Guild role update */
struct guild_role_update_t : public event_dispatch_t {
	guild_role_update_t(const std::string& raw);
	guild* updating_guild;
	role* updated;
};

/* Guild role delete */
struct guild_role_delete_t : public event_dispatch_t {
	guild_role_delete_t(const std::string& raw);
	guild* deleting_guild;
	role* deleted;
};

/* Channel pins update */
struct channel_pins_update_t : public event_dispatch_t {
	channel_pins_update_t(const std::string& raw);
	guild* pin_guild;
	channel* pin_channel;
	time_t timestamp;
};

/* Message remove all reactions */
struct message_reaction_remove_all_t : public event_dispatch_t {
	message_reaction_remove_all_t(const std::string& raw);
        guild* reacting_guild;
        channel* reacting_channel;
        snowflake message_id;
};

struct voice_server_update_t : public event_dispatch_t {
	voice_server_update_t(const std::string& raw);
};

/* Guild emojis update */
struct guild_emojis_update_t : public event_dispatch_t {
	guild_emojis_update_t(const std::string& raw);
	std::vector<snowflake> emojis;
	guild* updating_guild;
};

struct presence_update_t : public event_dispatch_t {
	presence_update_t(const std::string& raw);
};

struct webhooks_update_t : public event_dispatch_t {
	webhooks_update_t(const std::string& raw);
};

/* Guild member add */
struct guild_member_add_t : public event_dispatch_t {
	guild_member_add_t(const std::string& raw);
	guild* adding_guild;
	guild_member* added;
};

struct invite_delete_t : public event_dispatch_t {
	invite_delete_t(const std::string& raw);
};

/* Guild update */
struct guild_update_t : public event_dispatch_t {
	guild_update_t(const std::string& raw);
	guild* updated;
};

struct guild_integrations_update_t : public event_dispatch_t {
	guild_integrations_update_t(const std::string& raw);
};

/* Guild member update */
struct guild_member_update_t : public event_dispatch_t {
	guild_member_update_t(const std::string& raw);
	guild* updating_guild;
	guild_member* updated;
};

struct application_command_update_t : public event_dispatch_t {
	application_command_update_t(const std::string& raw);
};

struct invite_create_t : public event_dispatch_t {
	invite_create_t(const std::string& raw);
};

/* Message update */
struct message_update_t : public event_dispatch_t {
	message_update_t(const std::string& raw);
	message* updated;
};

/* User update */
struct user_update_t : public event_dispatch_t {
	user_update_t(const std::string& raw);
	user* updated;
};

/** Create message */
struct message_create_t : public event_dispatch_t {
	message_create_t(const std::string& raw);
	message* msg;
};

/* Guild ban add */
struct guild_ban_add_t : public event_dispatch_t {
	guild_ban_add_t(const std::string& raw);
	guild* banning_guild;
	user banned;
};

/* Guild ban remove */
struct guild_ban_remove_t : public event_dispatch_t {
	guild_ban_remove_t(const std::string& raw);
	guild* unbanning_guild;
	user unbanned;
};

struct integration_create_t : public event_dispatch_t {
	integration_create_t(const std::string& raw);
};

struct integration_update_t : public event_dispatch_t {
	integration_update_t(const std::string& raw);
};

struct integration_delete_t : public event_dispatch_t {
	integration_delete_t(const std::string& raw);
};

/** The dispatcher class contains a set of std::functions representing hooked events
 * that the user code is interested in.
 */
class dispatcher {
public:
	std::function<void(const voice_state_update_t& event)> voice_state_update;
	std::function<void(const interaction_create_t& event)> interaction_create;
	std::function<void(const guild_delete_t& event)> guild_delete;
	std::function<void(const channel_delete_t& event)> channel_delete;
	std::function<void(const channel_update_t& event)> channel_update;
	std::function<void(const ready_t& event)> ready;
	std::function<void(const message_delete_t& event)> message_delete;
	std::function<void(const application_command_delete_t& event)> application_command_delete;
	std::function<void(const guild_member_remove_t& event)> guild_member_remove;
	std::function<void(const application_command_create_t& event)> application_command_create;
	std::function<void(const resumed_t& event)> resumed;
	std::function<void(const guild_role_create_t& event)> guild_role_create;
	std::function<void(const typing_start_t& event)> typing_start;
	std::function<void(const message_reaction_add_t& event)> message_reaction_add;
	std::function<void(const guild_members_chunk_t& event)> guild_members_chunk;
	std::function<void(const message_reaction_remove_t& event)> message_reaction_remove;
	std::function<void(const guild_create_t& event)> guild_create;
	std::function<void(const channel_create_t& event)> channel_create;
	std::function<void(const message_reaction_remove_emoji_t& event)> message_reaction_remove_emoji;
	std::function<void(const message_delete_bulk_t& event)> message_delete_bulk;
	std::function<void(const guild_role_update_t& event)> guild_role_update;
	std::function<void(const guild_role_delete_t& event)> guild_role_delete;
	std::function<void(const channel_pins_update_t& event)> channel_pins_update;
	std::function<void(const message_reaction_remove_all_t& event)> message_reaction_remove_all;
	std::function<void(const voice_server_update_t& event)> voice_server_update;
	std::function<void(const guild_emojis_update_t& event)> guild_emojis_update;
	std::function<void(const presence_update_t& event)> presence_update;
	std::function<void(const webhooks_update_t& event)> webhooks_update;
	std::function<void(const guild_member_add_t& event)> guild_member_add;
	std::function<void(const invite_delete_t& event)> invite_delete;
	std::function<void(const guild_update_t& event)> guild_update;
	std::function<void(const guild_integrations_update_t& event)> guild_integrations_update;
	std::function<void(const guild_member_update_t& event)> guild_member_update;
	std::function<void(const application_command_update_t& event)> application_command_update;
	std::function<void(const invite_create_t& event)> invite_create;
	std::function<void(const message_update_t& event)> message_update;
	std::function<void(const user_update_t& event)> user_update;
	std::function<void(const message_create_t& event)> message_create;
	std::function<void(const guild_ban_add_t& event)> guild_ban_add;
	std::function<void(const guild_ban_remove_t& event)> guild_ban_remove;
	std::function<void(const integration_create_t& event)> integration_create;
	std::function<void(const integration_update_t& event)> integration_update;
	std::function<void(const integration_delete_t& event)> integration_delete;
};

};

