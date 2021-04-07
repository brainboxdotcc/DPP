#pragma once

#include <dpp/discord.h>
#include <dpp/message.h>
#include <functional>

namespace dpp {

struct voice_state_update_t {
};

struct interaction_create_t {
};

/** Delete guild */
struct guild_delete_t {
	guild* deleted;
};

/** Delete channel */
struct channel_delete_t {
	guild* deleting_guild;
	channel* deleted;
};

/** Update channel */
struct channel_update_t {
	guild* updating_guild;
	channel* updated;
};

/** Session ready */
struct ready_t {
	std::string session_id;
	uint32_t shard_id;
};

/* Message Deleted */
struct message_delete_t {
	message* deleted;
};

struct application_command_delete_t {
};

/* Guild member remove */
struct guild_member_remove_t {
	guild* removing_guild;
	user* removed;
};

struct application_command_create_t {
};

/** Session resumed */
struct resumed_t {
	std::string session_id;
	uint32_t shard_id;
};

struct guild_role_create_t {
	role* created;
};

struct typing_start_t {
};

struct message_reaction_add_t {
};

struct guild_members_chunk_t {
};

struct message_reaction_remove_t {
};

/** Create guild */
struct guild_create_t {
	guild* created;
};

/** Create channel */
struct channel_create_t {
	guild* creating_guild;
	channel* created;
};

struct message_reaction_remove_emoji_t {
};

struct message_delete_bulk_t {
};

struct guild_role_update_t {
	guild* updating_guild;
	role* updated;
};

struct guild_role_delete_t {
	guild* deleting_guild;
	role* deleted;
};

struct channel_pins_update_t {
};

struct message_reaction_remove_all_t {
};

struct voice_server_update_t {
};

struct guild_emojis_update_t {
};

struct presence_update_t {
};

struct webhooks_update_t {
};

/* Guild member add */
struct guild_member_add_t {
	guild* adding_guild;
	guild_member* added;
};

struct invite_delete_t {
};

struct guild_update_t {
	guild* updated;
};

struct guild_integrations_update_t {
};

struct guild_member_update_t {
	guild* updating_guild;
	guild_member* updated;
};

struct application_command_update_t {
};

struct invite_create_t {
};

/* Message update */
struct message_update_t {
	message* updated;
};

/* User update */
struct user_update_t {
	user* updated;
};

/** Create message */
struct message_create_t {
	message* msg;
};

struct guild_ban_add_t {
	guild* banning_guild;
};

struct integration_create_t {
};

struct integration_update_t {
};

struct integration_delete_t {
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
	std::function<void(const integration_create_t& event)> integration_create;
	std::function<void(const integration_update_t& event)> integration_update;
	std::function<void(const integration_delete_t& event)> integration_delete;
};

};

