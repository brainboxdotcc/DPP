/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <dpp/appcommand.h>
#include <dpp/message.h>
#include <dpp/discordclient.h>
#include <dpp/dispatcher.h>
#include <dpp/cluster.h>
#include <variant>

#define event_ctor(a, b) a::a(discord_client* client, const std::string &raw) : b(client, raw) {}

namespace dpp {

thread_local bool stop_event = false;

event_dispatch_t::event_dispatch_t(discord_client* client, const std::string &raw) : raw_event(raw), from(client)
{
	/* NOTE: This is thread_local because the event_dispatch_t sent to the event is const and cannot itself be modified,
	 * so there's no const-safe way to set an object variable to true later!
	 */
	stop_event = false;
}

const event_dispatch_t& event_dispatch_t::cancel_event() const
{
	/* NOTE: This is thread_local because the event_dispatch_t sent to the event is const and cannot itself be modified,
	 * so there's no const-safe way to have this as an object property!
	 */
	stop_event = true;
	return *this;
}

bool event_dispatch_t::is_cancelled() const
{
	return stop_event;
}

context_menu_t::context_menu_t(class discord_client* client, const std::string& raw) : interaction_create_t(client, raw) {
}

message_context_menu_t::message_context_menu_t(class discord_client* client, const std::string& raw) : context_menu_t(client, raw) {
}

message message_context_menu_t::get_message() const {
	return ctx_message;
}

message_context_menu_t& message_context_menu_t::set_message(const message& m) {
	ctx_message = m;
	return *this;
}

user_context_menu_t::user_context_menu_t(class discord_client* client, const std::string& raw) : context_menu_t(client, raw) {
}

user user_context_menu_t::get_user() const {
	return ctx_user;
}

user_context_menu_t& user_context_menu_t::set_user(const user& u) {
	ctx_user = u;
	return *this;
}


void message_create_t::send(const std::string& m, command_completion_event_t callback) const
{
	this->send(dpp::message(m), callback);
}

void message_create_t::send(message& msg, command_completion_event_t callback) const 
{
	msg.channel_id = this->msg.channel_id;
	this->from->creator->message_create(msg, callback);
}

void message_create_t::send(message&& msg, command_completion_event_t callback) const 
{
	msg.channel_id = this->msg.channel_id;
	this->from->creator->message_create(msg, callback);
}

void message_create_t::reply(const std::string& m, bool mention_replied_user, command_completion_event_t callback) const
{
	this->reply(dpp::message(m), mention_replied_user, callback);
}

void message_create_t::reply(message& msg, bool mention_replied_user, command_completion_event_t callback) const 
{
	msg.set_reference(this->msg.id);
	msg.channel_id = this->msg.channel_id;
	if (mention_replied_user) {
		msg.allowed_mentions.replied_user = mention_replied_user;
		msg.allowed_mentions.users.push_back(this->msg.author.id);
	}
	this->from->creator->message_create(msg, callback);
}

void message_create_t::reply(message&& msg, bool mention_replied_user, command_completion_event_t callback) const 
{
	msg.set_reference(this->msg.id);
	msg.channel_id = this->msg.channel_id;
	if (mention_replied_user) {
		msg.allowed_mentions.replied_user = mention_replied_user;
		msg.allowed_mentions.users.push_back(this->msg.author.id);
	}
	this->from->creator->message_create(msg, callback);
}

void interaction_create_t::reply(interaction_response_type t, const message & m, command_completion_event_t callback) const
{
	from->creator->interaction_response_create(this->command.id, this->command.token, dpp::interaction_response(t, m), callback);
}

void interaction_create_t::reply(const message & m, command_completion_event_t callback) const
{
	from->creator->interaction_response_create(
		this->command.id,
		this->command.token,
		dpp::interaction_response(ir_channel_message_with_source, m),
		callback
	);
}

void interaction_create_t::thinking(bool ephemeral, command_completion_event_t callback) const {
	message msg{this->command.channel_id, std::string{"*"}};
	msg.guild_id = this->command.guild_id;
	if (ephemeral) {
		msg.set_flags(dpp::m_ephemeral);
	}
	this->reply(ir_deferred_channel_message_with_source, msg, callback);
}

void interaction_create_t::reply(command_completion_event_t callback) const
{
	this->reply(ir_deferred_update_message, message(), callback);
}

void interaction_create_t::dialog(const interaction_modal_response& mr, command_completion_event_t callback) const
{
	from->creator->interaction_response_create(this->command.id, this->command.token, mr, callback);
}

void interaction_create_t::reply(interaction_response_type t, const std::string & mt, command_completion_event_t callback) const
{
	this->reply(t, dpp::message(this->command.channel_id, mt, mt_application_command), callback);
}

void interaction_create_t::reply(const std::string & mt, command_completion_event_t callback) const
{
	this->reply(ir_channel_message_with_source, dpp::message(this->command.channel_id, mt, mt_application_command), callback);
}

void interaction_create_t::edit_response(const message & m, command_completion_event_t callback) const
{
	from->creator->interaction_response_edit(this->command.token, m, callback);
}

void interaction_create_t::edit_response(const std::string & mt, command_completion_event_t callback) const
{
	this->edit_response(dpp::message(this->command.channel_id, mt, mt_application_command), callback);
}

void interaction_create_t::get_original_response(command_completion_event_t callback) const
{
	from->creator->post_rest(API_PATH "/webhooks", std::to_string(command.application_id), command.token + "/messages/@original", m_get, "", [creator = this->from->creator, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t(creator, message().fill_from_json(&j), http));
		}
	});
}

void interaction_create_t::edit_original_response(const message & m, command_completion_event_t callback) const
{
	from->creator->post_rest_multipart(API_PATH "/webhooks", std::to_string(command.application_id), command.token + "/messages/@original", m_patch, m.build_json(), [creator = this->from->creator, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t(creator, message().fill_from_json(&j), http));
		}
	}, m.filename, m.filecontent, m.filemimetype);
}

void interaction_create_t::delete_original_response(command_completion_event_t callback) const
{
	from->creator->post_rest(API_PATH "/webhooks", std::to_string(command.application_id), command.token + "/messages/@original", m_delete, "", [creator = this->from->creator, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t(creator, confirmation(), http));
		}
	});
}


#ifdef DPP_CORO
async<confirmation_callback_t> interaction_create_t::co_reply() const {
	return dpp::async{[this] <typename T> (T&& cb) { this->reply(std::forward<T>(cb)); }};
}

async<confirmation_callback_t> interaction_create_t::co_reply(interaction_response_type t, const message & m) const {
	return dpp::async{[&, this] <typename T> (T&& cb) { this->reply(t, m, std::forward<T>(cb)); }};
}

async<confirmation_callback_t> interaction_create_t::co_reply(interaction_response_type t, const std::string & mt) const {
	return dpp::async{[&, this] <typename T> (T&& cb) { this->reply(t, mt, std::forward<T>(cb)); }};
}

async<confirmation_callback_t> interaction_create_t::co_reply(const message & m) const {
	return dpp::async{[&, this] <typename T> (T&& cb) { this->reply(m, std::forward<T>(cb)); }};
}

async<confirmation_callback_t> interaction_create_t::co_reply(const std::string & mt) const {
	return dpp::async{[&, this] <typename T> (T&& cb) { this->reply(mt, std::forward<T>(cb)); }};
}

async<confirmation_callback_t> interaction_create_t::co_dialog(const interaction_modal_response& mr) const {
	return dpp::async{[&, this] <typename T> (T&& cb) { this->dialog(mr, std::forward<T>(cb)); }};
}

async<confirmation_callback_t> interaction_create_t::co_edit_response(const message & m) const {
	return dpp::async{[&, this] <typename T> (T&& cb) { this->edit_response(m, std::forward<T>(cb)); }};
}

async<confirmation_callback_t> interaction_create_t::co_edit_response(const std::string & mt) const {
	return dpp::async{[&, this] <typename T> (T&& cb) { this->edit_response(mt, std::forward<T>(cb)); }};
}

async<confirmation_callback_t> interaction_create_t::co_thinking(bool ephemeral) const {
	return dpp::async{[&, this] <typename T> (T&& cb) { this->thinking(ephemeral, std::forward<T>(cb)); }};
}

async<confirmation_callback_t> interaction_create_t::co_get_original_response() const {
	return dpp::async{[&, this] <typename T> (T&& cb) { this->get_original_response(std::forward<T>(cb)); }};
}

async<confirmation_callback_t> interaction_create_t::co_edit_original_response(const message & m) const {
	return dpp::async{[&, this] <typename T> (T&& cb) { this->edit_original_response(m, std::forward<T>(cb)); }};
}

async<confirmation_callback_t> interaction_create_t::co_delete_original_response() const {
	return dpp::async{[&, this] <typename T> (T&& cb) { this->delete_original_response(std::forward<T>(cb)); }};
}
#endif /* DPP_CORO */

command_value interaction_create_t::get_parameter(const std::string& name) const
{
	const command_interaction ci = command.get_command_interaction();

	for (const auto &option : ci.options) {
		if (option.type != co_sub_command && option.type != co_sub_command_group && option.name == name) {
			return option.value;
		}
	}
	/* if not found in the first level, go one level deeper */
	for (const auto &option : ci.options) { // command
		for (const auto &sub_option : option.options) { // subcommands
			if (sub_option.type != co_sub_command && sub_option.type != co_sub_command_group && sub_option.name == name) {
				return sub_option.value;
			}
		}
	}
	/* if not found in the second level, search it in the third dimension */
	for (const auto &option : ci.options) { // command
		for (const auto &sub_group_option : option.options) { // subcommand groups
			for (const auto &sub_option : sub_group_option.options) { // subcommands
				if (sub_option.type != co_sub_command && sub_option.type != co_sub_command_group && sub_option.name == name) {
					return sub_option.value;
				}
			}
		}
	}
	return {};
}

voice_receive_t::voice_receive_t(class discord_client* client, const std::string &raw, class discord_voice_client* vc, snowflake _user_id, uint8_t* pcm, size_t length) : event_dispatch_t(client, raw), voice_client(vc), user_id(_user_id) {
	reassign(vc, _user_id, pcm, length);
}

void voice_receive_t::reassign(class discord_voice_client* vc, snowflake _user_id, uint8_t* pcm, size_t length) {
	voice_client = vc;
	user_id = _user_id;

	audio_data.assign(pcm, length);

	// for backwards compatibility; remove soon
	audio = audio_data.data();
	audio_size = audio_data.length();
	
}


/* Standard default constructors that call the parent constructor, for events */
event_ctor(guild_join_request_delete_t, event_dispatch_t);
event_ctor(stage_instance_create_t, event_dispatch_t);
event_ctor(stage_instance_update_t, event_dispatch_t);
event_ctor(stage_instance_delete_t, event_dispatch_t);
event_ctor(log_t, event_dispatch_t);
event_ctor(voice_state_update_t, event_dispatch_t);
event_ctor(interaction_create_t, event_dispatch_t);
event_ctor(button_click_t, interaction_create_t);
event_ctor(autocomplete_t, interaction_create_t);
event_ctor(select_click_t, interaction_create_t);
event_ctor(form_submit_t, interaction_create_t);
event_ctor(slashcommand_t, interaction_create_t);
event_ctor(guild_delete_t, event_dispatch_t);
event_ctor(channel_delete_t, event_dispatch_t);
event_ctor(channel_update_t, event_dispatch_t);
event_ctor(ready_t, event_dispatch_t);
event_ctor(message_delete_t, event_dispatch_t);
event_ctor(resumed_t, event_dispatch_t);
event_ctor(guild_role_create_t, event_dispatch_t);
event_ctor(typing_start_t, event_dispatch_t);
event_ctor(message_reaction_add_t, event_dispatch_t);
event_ctor(message_reaction_remove_t, event_dispatch_t);
event_ctor(guild_create_t, event_dispatch_t);
event_ctor(channel_create_t, event_dispatch_t);
event_ctor(message_reaction_remove_emoji_t, event_dispatch_t);
event_ctor(message_delete_bulk_t, event_dispatch_t);
event_ctor(guild_role_update_t, event_dispatch_t);
event_ctor(guild_role_delete_t, event_dispatch_t);
event_ctor(channel_pins_update_t, event_dispatch_t);
event_ctor(message_reaction_remove_all_t, event_dispatch_t);
event_ctor(voice_server_update_t, event_dispatch_t);
event_ctor(guild_emojis_update_t, event_dispatch_t);
event_ctor(presence_update_t, event_dispatch_t);
event_ctor(webhooks_update_t, event_dispatch_t);
event_ctor(guild_member_add_t, event_dispatch_t);
event_ctor(invite_delete_t, event_dispatch_t);
event_ctor(guild_update_t, event_dispatch_t);
event_ctor(guild_integrations_update_t, event_dispatch_t);
event_ctor(guild_member_update_t, event_dispatch_t);
event_ctor(invite_create_t, event_dispatch_t);
event_ctor(message_update_t, event_dispatch_t);
event_ctor(user_update_t, event_dispatch_t);
event_ctor(message_create_t, event_dispatch_t);
event_ctor(guild_audit_log_entry_create_t, event_dispatch_t);
event_ctor(guild_ban_add_t, event_dispatch_t);
event_ctor(guild_ban_remove_t, event_dispatch_t);
event_ctor(integration_create_t, event_dispatch_t);
event_ctor(integration_update_t, event_dispatch_t);
event_ctor(integration_delete_t, event_dispatch_t);
event_ctor(guild_member_remove_t, event_dispatch_t);
event_ctor(guild_members_chunk_t, event_dispatch_t);
event_ctor(thread_create_t, event_dispatch_t);
event_ctor(thread_update_t, event_dispatch_t);
event_ctor(thread_delete_t, event_dispatch_t);
event_ctor(thread_list_sync_t, event_dispatch_t);
event_ctor(thread_member_update_t, event_dispatch_t);
event_ctor(thread_members_update_t, event_dispatch_t);
event_ctor(voice_buffer_send_t, event_dispatch_t);
event_ctor(voice_user_talking_t, event_dispatch_t);
event_ctor(voice_ready_t, event_dispatch_t);
event_ctor(voice_receive_t, event_dispatch_t);
event_ctor(voice_client_speaking_t, event_dispatch_t);
event_ctor(voice_client_disconnect_t, event_dispatch_t);
event_ctor(voice_track_marker_t, event_dispatch_t);
event_ctor(guild_stickers_update_t, event_dispatch_t);
event_ctor(guild_scheduled_event_create_t, event_dispatch_t);
event_ctor(guild_scheduled_event_update_t, event_dispatch_t);
event_ctor(guild_scheduled_event_delete_t, event_dispatch_t);
event_ctor(guild_scheduled_event_user_add_t, event_dispatch_t);
event_ctor(guild_scheduled_event_user_remove_t, event_dispatch_t);
event_ctor(automod_rule_create_t, event_dispatch_t);
event_ctor(automod_rule_delete_t, event_dispatch_t);
event_ctor(automod_rule_update_t, event_dispatch_t);
event_ctor(automod_rule_execute_t, event_dispatch_t);
} // namespace dpp
