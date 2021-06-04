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
#include <dpp/discord.h>
#include <dpp/slashcommand.h>
#include <dpp/dispatcher.h>
#include <dpp/cluster.h>
#include <dpp/fmt/format.h>
#include <variant>

namespace dpp {

event_dispatch_t::event_dispatch_t(discord_client* client, const std::string &raw) : from(client), raw_event(raw)
{
}

guild_join_request_delete_t::guild_join_request_delete_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

stage_instance_create_t::stage_instance_create_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

stage_instance_delete_t::stage_instance_delete_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

log_t::log_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

voice_state_update_t::voice_state_update_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

interaction_create_t::interaction_create_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

void interaction_create_t::reply(interaction_response_type t, const message & m) const
{
	from->creator->interaction_response_create(this->command.id, this->command.token, dpp::interaction_response(t, m));
}

void interaction_create_t::reply(interaction_response_type t, const std::string & mt) const
{
	this->reply(t, dpp::message(this->command.channel_id, mt, mt_application_command));
}

const command_value& interaction_create_t::get_parameter(const std::string& name) const
{
	/* Dummy STATIC return value for unknown options so we arent returning a value off the stack */
	static command_value dummy_value = {};
	const command_interaction& ci = std::get<command_interaction>(command.data);
	for (auto i = ci.options.begin(); i != ci.options.end(); ++i) {
		if (i->name == name) {
			return i->value;
		}
	}
	return dummy_value;
}

button_click_t::button_click_t(discord_client* client, const std::string &raw) : interaction_create_t(client, raw)
{
}

const command_value& button_click_t::get_parameter(const std::string& name) const
{
	/* Buttons don't have parameters, so override this */
	static command_value dummy_b_value = {};
	return dummy_b_value;
}

guild_delete_t::guild_delete_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

channel_delete_t::channel_delete_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

channel_update_t::channel_update_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

ready_t::ready_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

message_delete_t::message_delete_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

application_command_delete_t::application_command_delete_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

application_command_create_t::application_command_create_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

resumed_t::resumed_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

guild_role_create_t::guild_role_create_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

typing_start_t::typing_start_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

message_reaction_add_t::message_reaction_add_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

message_reaction_remove_t::message_reaction_remove_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

guild_create_t::guild_create_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

channel_create_t::channel_create_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

message_reaction_remove_emoji_t::message_reaction_remove_emoji_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

message_delete_bulk_t::message_delete_bulk_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

guild_role_update_t::guild_role_update_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

guild_role_delete_t::guild_role_delete_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

channel_pins_update_t::channel_pins_update_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

message_reaction_remove_all_t::message_reaction_remove_all_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

voice_server_update_t::voice_server_update_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

guild_emojis_update_t::guild_emojis_update_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

presence_update_t::presence_update_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

webhooks_update_t::webhooks_update_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

guild_member_add_t::guild_member_add_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

invite_delete_t::invite_delete_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

guild_update_t::guild_update_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

guild_integrations_update_t::guild_integrations_update_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

guild_member_update_t::guild_member_update_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

application_command_update_t::application_command_update_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

invite_create_t::invite_create_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

message_update_t::message_update_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

user_update_t::user_update_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

message_create_t::message_create_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

guild_ban_add_t::guild_ban_add_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

guild_ban_remove_t::guild_ban_remove_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

integration_create_t::integration_create_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

integration_update_t::integration_update_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

integration_delete_t::integration_delete_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

guild_member_remove_t::guild_member_remove_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

guild_members_chunk_t::guild_members_chunk_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

voice_buffer_send_t::voice_buffer_send_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

voice_user_talking_t::voice_user_talking_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

voice_ready_t::voice_ready_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

voice_receive_t::voice_receive_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

voice_track_marker_t::voice_track_marker_t(discord_client* client, const std::string &raw) : event_dispatch_t(client, raw)
{
}

};
