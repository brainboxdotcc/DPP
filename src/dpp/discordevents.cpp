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
#define _XOPEN_SOURCE
#include <string>
#include <iostream>
#include <fstream>
#include <time.h>
#include <stdlib.h>
#include <dpp/discordclient.h>
#include <dpp/event.h>
#include <dpp/cache.h>
#include <dpp/stringops.h>
#include <dpp/nlohmann/json.hpp>
#include <time.h>
#include <iomanip>
#include <sstream>

char* crossplatform_strptime(const char* s, const char* f, struct tm* tm) {
	std::istringstream input(s);
	input.imbue(std::locale(setlocale(LC_ALL, nullptr)));
	input >> std::get_time(tm, f);
	if (input.fail()) {
		return nullptr;
	}
	return (char*)(s + input.tellg());
}

namespace dpp {

double managed::get_creation_time() const {
	return (double)((this->id >> 22) + 1420070400000) / 1000.0;
}


std::string ts_to_string(time_t ts) {
	std::ostringstream ss;
	struct tm t;
	#ifdef _WIN32
		gmtime_s(&t, &ts);
	#else
		gmtime_r(&ts, &t);
	#endif
	ss << std::put_time(&t, "%FT%TZ");
	return ss.str();
}

uint64_t snowflake_not_null(const json* j, const char *keyname) {
	/* Snowflakes are a special case. Pun intended.
	 * Because discord drinks the javascript kool-aid, they have to send 64 bit integers as strings as js can't deal with them
	 * even though we can. So, all snowflakes are sent and received wrapped as string values and must be read by nlohmann::json
	 * as string types, then converted from string to uint64_t. Checks for existence of the value, and that it is a string containing
	 * a number. If not, then this function returns 0.
	 */
	auto k = j->find(keyname);
	if (k != j->end()) {
		return !k->is_null() && k->is_string() ? strtoull(k->get<std::string>().c_str(), nullptr, 10) : 0;
	} else {
		return 0;
	}
}

void set_snowflake_not_null(const json* j, const char *keyname, uint64_t &v) {
	auto k = j->find(keyname);
	if (k != j->end()) {
		v = !k->is_null() && k->is_string() ? strtoull(k->get<std::string>().c_str(), nullptr, 10) : 0;
	}
}


std::string string_not_null(const json* j, const char *keyname) {
	/* Returns empty string if the value is not a string, or is null or not defined */
	auto k = j->find(keyname);
	if (k != j->end()) {
		return !k->is_null() && k->is_string() ? k->get<std::string>() : "";
	} else {
		return const_cast< char* >("");
	}
}

void set_string_not_null(const json* j, const char *keyname, std::string &v) {
	/* Returns empty string if the value is not a string, or is null or not defined */
	auto k = j->find(keyname);
	if (k != j->end()) {
		v = !k->is_null() && k->is_string() ? k->get<std::string>() : "";
	}
}

double double_not_null(const json* j, const char *keyname) {
	auto k = j->find(keyname);
	if (k != j->end()) {
		return !k->is_null() && !k->is_string() ? k->get<double>() : 0;
	} else {
		return 0;
	}
}

void set_double_not_null(const json* j, const char *keyname, double &v) {
	auto k = j->find(keyname);
	if (k != j->end()) {
		v = !k->is_null() && !k->is_string() ? k->get<double>() : 0;
	}
}

uint64_t int64_not_null(const json* j, const char *keyname) {
	auto k = j->find(keyname);
	if (k != j->end()) {
		return !k->is_null() && !k->is_string() ? k->get<uint64_t>() : 0;
	} else {
		return 0;
	}
}

void set_int64_not_null(const json* j, const char *keyname, uint64_t &v) {
	auto k = j->find(keyname);
	if (k != j->end()) {
		v = !k->is_null() && !k->is_string() ? k->get<uint64_t>() : 0;
	}
}


uint32_t int32_not_null(const json* j, const char *keyname) {
	auto k = j->find(keyname);
	if (k != j->end()) {
		return !k->is_null() && !k->is_string() ? k->get<uint32_t>() : 0;
	} else {
		return 0;
	}
}

void set_int32_not_null(const json* j, const char *keyname, uint32_t &v) {
	auto k = j->find(keyname);
	if (k != j->end()) {
		v = !k->is_null() && !k->is_string() ? k->get<uint32_t>() : 0;
	}
}

uint16_t int16_not_null(const json* j, const char *keyname) {
	auto k = j->find(keyname);
	if (k != j->end()) {
		return !k->is_null() && !k->is_string() ? k->get<uint16_t>() : 0;
	} else {
		return 0;
	}
}

void set_int16_not_null(const json* j, const char *keyname, uint16_t &v) {
	auto k = j->find(keyname);
	if (k != j->end()) {
		v = !k->is_null() && !k->is_string() ? k->get<uint16_t>() : 0;
	}
}

uint8_t int8_not_null(const json* j, const char *keyname) {
	auto k = j->find(keyname);
	if (k != j->end()) {
		return !k->is_null() && !k->is_string() ? k->get<uint8_t>() : 0;
	} else {
		return 0;
	}
}

void set_int8_not_null(const json* j, const char *keyname, uint8_t &v) {
	auto k = j->find(keyname);
	if (k != j->end()) {
		v = !k->is_null() && !k->is_string() ? k->get<uint8_t>() : 0;
	}
}

bool bool_not_null(const json* j, const char *keyname) {
	auto k = j->find(keyname);
	if (k != j->end()) {
		return !k->is_null() ? (k->get<bool>() == true) : false;
	} else {
		return false;
	}
}

void set_bool_not_null(const json* j, const char *keyname, bool &v) {
	auto k = j->find(keyname);
	if (k != j->end()) {
		v = !k->is_null() ? (k->get<bool>() == true) : false;
	}
}

std::string base64_encode(unsigned char const* buf, unsigned int buffer_length) {
	/* Quick and dirty base64 encode */
	static const char to_base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	size_t ret_size = buffer_length + 2;

	ret_size = 4 * ret_size / 3;

	std::string ret;
	ret.reserve(ret_size);

	for (unsigned int i=0; i<ret_size/4; ++i)
	{
		size_t index = i*3;
		unsigned char b3[3];
		b3[0] = buf[index+0];
		b3[1] = buf[index+1];
		b3[2] = buf[index+2];

		ret.push_back(to_base64[ ((b3[0] & 0xfc) >> 2) ]);
		ret.push_back(to_base64[ ((b3[0] & 0x03) << 4) + ((b3[1] & 0xf0) >> 4) ]);
		ret.push_back(to_base64[ ((b3[1] & 0x0f) << 2) + ((b3[2] & 0xc0) >> 6) ]);
		ret.push_back(to_base64[ ((b3[2] & 0x3f)) ]);
	}

	return ret;
}

time_t ts_not_null(const json* j, const char* keyname)
{
	/* Parses discord ISO 8061 timestamps to time_t, accounting for local time adjustment.
	 * Note that discord timestamps contain a decimal seconds part, which time_t and struct tm
	 * can't handle. We strip these out.
	 */
	time_t retval = 0;
	if (j->find(keyname) != j->end() && !(*j)[keyname].is_null() && (*j)[keyname].is_string()) {
		tm timestamp = {};
		std::string timedate = (*j)[keyname].get<std::string>();
		if (timedate.find('+') != std::string::npos) {
			std::string tzpart = timedate.substr(timedate.find('+'), timedate.length());
			if (timedate.find('.') != std::string::npos) {
				timedate = timedate.substr(0, timedate.find('.')); // + "Z" + tzpart;
			}
			crossplatform_strptime(timedate.substr(0, 19).c_str(), "%Y-%m-%dT%T", &timestamp);
			timestamp.tm_isdst = 0;
			retval = mktime(&timestamp);
		} else {
			crossplatform_strptime(timedate.substr(0, 19).c_str(), "%Y-%m-%d %T", &timestamp);
			retval = mktime(&timestamp);
		}
	}
	return retval;
}

void set_ts_not_null(const json* j, const char* keyname, time_t &v)
{
	/* Parses discord ISO 8061 timestamps to time_t, accounting for local time adjustment.
	 * Note that discord timestamps contain a decimal seconds part, which time_t and struct tm
	 * can't handle. We strip these out.
	 */
	time_t retval = 0;
	if (j->find(keyname) != j->end() && !(*j)[keyname].is_null() && (*j)[keyname].is_string()) {
		tm timestamp = {};
		std::string timedate = (*j)[keyname].get<std::string>();
		if (timedate.find('+') != std::string::npos) {
			std::string tzpart = timedate.substr(timedate.find('+'), timedate.length());
			if (timedate.find('.') != std::string::npos) {
				timedate = timedate.substr(0, timedate.find('.')); // + "Z" + tzpart;
			}
			crossplatform_strptime(timedate.substr(0, 19).c_str(), "%Y-%m-%dT%T", &timestamp);
			timestamp.tm_isdst = 0;
			retval = mktime(&timestamp);
		} else {
			crossplatform_strptime(timedate.substr(0, 19).c_str(), "%Y-%m-%d %T", &timestamp);
			retval = mktime(&timestamp);
		}
		v = retval;
	}
}

const std::map<std::string, dpp::events::event*> eventmap = {
	{ "__LOG__", new dpp::events::logger() },
	{ "GUILD_CREATE", new dpp::events::guild_create() },
	{ "GUILD_UPDATE", new dpp::events::guild_update() },
	{ "GUILD_DELETE", new dpp::events::guild_delete() },
	{ "GUILD_MEMBER_UPDATE", new dpp::events::guild_member_update() },
	{ "RESUMED", new dpp::events::resumed() },
	{ "READY", new dpp::events::ready() },
	{ "CHANNEL_CREATE", new dpp::events::channel_create() },
	{ "CHANNEL_UPDATE", new dpp::events::channel_update() },
	{ "CHANNEL_DELETE", new dpp::events::channel_delete() },
	{ "PRESENCE_UPDATE", new dpp::events::presence_update() },
	{ "TYPING_START", new dpp::events::typing_start() },
	{ "MESSAGE_CREATE", new dpp::events::message_create() },
	{ "MESSAGE_UPDATE", new dpp::events::message_update() },
	{ "MESSAGE_DELETE", new dpp::events::message_delete() },
	{ "MESSAGE_DELETE_BULK", new dpp::events::message_delete_bulk() },
	{ "MESSAGE_REACTION_ADD", new dpp::events::message_reaction_add() },
	{ "MESSAGE_REACTION_REMOVE", new dpp::events::message_reaction_remove() },
	{ "MESSAGE_REACTION_REMOVE_ALL", new dpp::events::message_reaction_remove_all() },
	{ "MESSAGE_REACTION_REMOVE_EMOJI", new dpp::events::message_reaction_remove_emoji() },
	{ "CHANNEL_PINS_UPDATE", new dpp::events::channel_pins_update() },
	{ "GUILD_BAN_ADD", new dpp::events::guild_ban_add() },
	{ "GUILD_BAN_REMOVE", new dpp::events::guild_ban_remove() },
	{ "GUILD_EMOJIS_UPDATE", new dpp::events::guild_emojis_update() },
	{ "GUILD_INTEGRATIONS_UPDATE", new dpp::events::guild_integrations_update() },
	{ "INTEGRATION_CREATE", new dpp::events::integration_create() },
	{ "INTEGRATION_UPDATE", new dpp::events::integration_update() },
	{ "INTEGRATION_DELETE", new dpp::events::integration_delete() },
	{ "GUILD_MEMBER_ADD", new dpp::events::guild_member_add() },
	{ "GUILD_MEMBER_REMOVE", new dpp::events::guild_member_remove() },
	{ "GUILD_MEMBERS_CHUNK", new dpp::events::guild_members_chunk() },
	{ "GUILD_ROLE_CREATE", new dpp::events::guild_role_create() },
	{ "GUILD_ROLE_UPDATE", new dpp::events::guild_role_update() },
	{ "GUILD_ROLE_DELETE", new dpp::events::guild_role_delete() },
	{ "VOICE_STATE_UPDATE", new dpp::events::voice_state_update() },
	{ "VOICE_SERVER_UPDATE", new dpp::events::voice_server_update() },
	{ "WEBHOOKS_UPDATE", new dpp::events::webhooks_update() },
	{ "INVITE_CREATE", new dpp::events::invite_create() },
	{ "INVITE_DELETE", new dpp::events::invite_delete() },
	{ "INTERACTION_CREATE", new dpp::events::interaction_create() },
	{ "USER_UPDATE", new dpp::events::user_update() },
	{ "GUILD_JOIN_REQUEST_DELETE", new dpp::events::guild_join_request_delete() },
	{ "STAGE_INSTANCE_CREATE", new dpp::events::stage_instance_create() },
	{ "STAGE_INSTANCE_UPDATE", new dpp::events::stage_instance_update() },
	{ "STAGE_INSTANCE_DELETE", new dpp::events::stage_instance_delete() },
	{ "THREAD_CREATE", new dpp::events::thread_create() },
	{ "THREAD_UPDATE", new dpp::events::thread_update() },
	{ "THREAD_DELETE", new dpp::events::thread_delete() },
	{ "THREAD_LIST_SYNC", new dpp::events::thread_list_sync() },
	{ "THREAD_MEMBER_UPDATE", new dpp::events::thread_member_update() },
	{ "THREAD_MEMBERS_UPDATE", new dpp::events::thread_members_update() },
	{ "GUILD_STICKERS_UPDATE", new dpp::events::guild_stickers_update() },
	{ "GUILD_APPLICATION_COMMAND_COUNTS_UPDATE", nullptr },
	{ "APPLICATION_COMMAND_PERMISSIONS_UPDATE", nullptr },
	{ "EMBEDDED_ACTIVITY_UPDATE", nullptr },
	{ "GUILD_SCHEDULED_EVENT_CREATE", new dpp::events::guild_scheduled_event_create() },
	{ "GUILD_SCHEDULED_EVENT_UPDATE", new dpp::events::guild_scheduled_event_update() },
	{ "GUILD_SCHEDULED_EVENT_DELETE", new dpp::events::guild_scheduled_event_delete() },
	{ "GUILD_SCHEDULED_EVENT_USER_ADD", new dpp::events::guild_scheduled_event_user_add() },
	{ "GUILD_SCHEDULED_EVENT_USER_REMOVE", new dpp::events::guild_scheduled_event_user_remove() },
};

void discord_client::handle_event(const std::string &event, json &j, const std::string &raw)
{
	auto ev_iter = eventmap.find(event);
	if (ev_iter != eventmap.end()) {
		/* A handler with nullptr is silently ignored. We don't plan to make a handler for it
		 * so this usually some user-only thing that's crept into the API and shown to bots
		 * that we dont care about.
		 */
		if (ev_iter->second != nullptr) {
			ev_iter->second->handle(this, j, raw);
		}
	} else {
		log(dpp::ll_debug, "Unhandled event: " + event + ", " + j.dump());
	}
}

};
