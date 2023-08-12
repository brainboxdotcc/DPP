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

#include <dpp/streams.h>
#include <dpp/channel.h>
#include <utility>

namespace dpp {

end_msg_t end_msg() { 
	return {}; 
}

end_row_t end_row() { 
	return {}; 
}



channel_stream::channel_stream(cluster& bot_, const channel& out_channel) : bot(&bot_), out_channel_id(out_channel.id) {}

channel_stream::channel_stream(cluster& bot_, snowflake out_channel_id_) : bot(&bot_), out_channel_id(out_channel_id_) {}

void channel_stream::send(command_completion_event_t callback) {
	this->msg.set_channel_id(this->out_channel_id); //Make sure message is sent to the right channel
	this->bot->message_create(this->msg,callback);
}

message channel_stream::send_sync() {
	this->msg.set_channel_id(this->out_channel_id);
	return this->bot->message_create_sync(this->msg);
}

dm_stream::dm_stream(cluster& bot_, const user& out_user) : bot(&bot_), out_user_id(out_user.id) {}

dm_stream::dm_stream(cluster& bot_, snowflake out_user_id_) : bot(&bot_), out_user_id(out_user_id_) {}

void dm_stream::send(command_completion_event_t callback) {
	
	this->bot->direct_message_create(this->out_user_id,this->msg,callback);
}

message dm_stream::send_sync() {
	
	return this->bot->direct_message_create_sync(out_user_id,this->msg);
}

void base_stream::add_component(const component& c) {
	if (this->n_rows >= 5) throw dpp::length_exception("Message already contains five action rows");
	component_type c_t = c.type;
	if (c_t == cot_action_row) {
		if (this->n_buttons > 0) this->add_row();
		this->current_action_row = c;
		this->add_row();
	} else if (c_t == cot_button) {
		this->current_action_row.add_component(c);
		this->n_buttons++;
		if (this->n_buttons >= 5) this->add_row();
	} else {
		if (this->n_buttons > 0) this->add_row();
		this->current_action_row = component().add_component(c);
		this->add_row();
	}
}
		

void base_stream::add_row() {		
	this->n_buttons = 0;
	this->msg.add_component(this->current_action_row);
	this->current_action_row = component();
	this->n_rows++;
}

base_stream& base_stream::operator<<(const std::string& msg) {
	this->msg.set_content(this->msg.content + msg);
	return (*this);
}
base_stream& base_stream::operator<<(end_msg_t) {
	if (this->n_buttons > 0) this->add_row();
	this->send();
	this->msg = message();
	return (*this);
}

base_stream& base_stream::operator<<(end_row_t) {
	this->add_row();
	return (*this);
}

base_stream& base_stream::operator<<(const embed& e) {
	this->msg.add_embed(e);
	return (*this);
}
base_stream& base_stream::operator<<(const component& c) {
	this->add_component(c);
	return (*this);
}


base_stream& base_stream::operator<<(const file_info &f) {
    this->msg.add_file(f.file_name,f.file_content,f.mime_type);
    return (*this);
}

} //namespace dpp
