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

int end_msg() { return 0; }
channel_stream::channel_stream(cluster& bot, channel out_channel) {
	this->bot = &bot;
	this->out_channel_id = out_channel.id;
}
channel_stream::channel_stream(cluster& bot, snowflake out_channel_id) {
	this->bot = &bot;
	this->out_channel_id = out_channel_id;
}
void channel_stream::send(command_completion_event_t callback) {
	this->msg.set_channel_id(this->out_channel_id); //Make sure message is sent to the right channel
	this->bot->message_create(this->msg,callback);
}
message channel_stream::send_sync() {
	this->msg.set_channel_id(this->out_channel_id);
	return this->bot->message_create_sync(this->msg);
}




dm_stream::dm_stream(cluster& bot, user out_user) {
	this->bot = &bot;
	this->out_user_id = out_user.id;
}
dm_stream::dm_stream(cluster& bot, snowflake out_user_id) {
	this->bot = &bot;
	this->out_user_id = out_user_id;
}
void dm_stream::send(command_completion_event_t callback) {
	
	this->bot->direct_message_create(this->out_user_id,this->msg,callback);
}
message dm_stream::send_sync() {
	
	return this->bot->direct_message_create_sync(out_user_id,this->msg);
}

base_stream& base_stream::operator<<(const std::string& msg) {
	this->msg.set_content(this->msg.content + msg);
	return (*this);
}
base_stream& base_stream::operator<<(const int& n) {
	this->send();
	this->msg = message();
	return (*this);
}
base_stream& base_stream::operator<<(const embed& e) {
	this->msg.add_embed(e);
	return (*this);
}
base_stream& base_stream::operator<<(const component& c) {
	this->msg.add_component(c);
	return (*this);
}

base_stream& base_stream::operator<<(const std::pair<std::string,std::string> &f) {
    this->msg.add_file(f.first,f.second);
    return (*this);
}
} //namespace dpp
