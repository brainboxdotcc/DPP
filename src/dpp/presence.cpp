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
#include <dpp/presence.h>
#include <dpp/discordevents.h>
#include <dpp/nlohmann/json.hpp>

using json = nlohmann::json;

namespace dpp {

activity::activity(const activity_type typ, const std::string& nam, const std::string& stat, const std::string& url_) :
	 name(nam), state(stat), url(url_), type(typ)
{	
}

presence::presence() : user_id(0), guild_id(0), flags(0)
{
}

presence::presence(presence_status status, activity_type type, const std::string& activity_description) {
	dpp::activity a;
	a.name = activity_description;
	a.type = type;
	activities.clear();
	activities.emplace_back(a);
	flags &= PF_CLEAR_STATUS;
	if (status == ps_online)
		flags |= p_status_online;
	else if (status == ps_idle)
		flags |= p_status_idle;
	else if (status == ps_dnd)
		flags |= p_status_dnd;
}

presence::presence(presence_status status, const activity &a) {
	activities.clear();
	activities.emplace_back(a);
	flags &= PF_CLEAR_STATUS;
	if (status == ps_online)
		flags |= p_status_online;
	else if (status == ps_idle)
		flags |= p_status_idle;
	else if (status == ps_dnd)
		flags |= p_status_dnd;
}

presence::~presence() = default;

presence& presence::fill_from_json(nlohmann::json* j) {
	guild_id = snowflake_not_null(j, "guild_id");
	user_id = snowflake_not_null(&((*j)["user"]), "id");

	auto f = j->find("client_status");
	if (f != j->end()) {

		bool update_desktop = false, update_web = false, update_mobile = false;
		std::string desktop, mobile, web;

		if (f->find("desktop") != f->end()) {
			desktop = string_not_null(&((*j)["client_status"]), "desktop");
			update_desktop = true;
		}
		if (f->find("mobile") != f->end()) {
			mobile = string_not_null(&((*j)["client_status"]), "mobile");
			update_mobile = true;
		}
		if (f->find("web") != f->end()) {
			web = string_not_null(&((*j)["client_status"]), "web");
			update_web = true;
		}

		if (update_desktop) {
			flags &= PF_CLEAR_DESKTOP;
			if (desktop == "online")
				flags |= p_desktop_online;
			else if (desktop == "idle")
				flags |= p_desktop_idle;
			else if (desktop == "dnd")
				flags |= p_desktop_dnd;
		}

		if (update_mobile) {
			flags &= PF_CLEAR_MOBILE;
			if (mobile == "online")
				flags |= p_mobile_online;
			else if (mobile == "idle")
				flags |= p_mobile_idle;
			else if (mobile == "dnd")
				flags |= p_mobile_dnd;
		}

		if (update_web) {
			flags &= PF_CLEAR_WEB;
			if (web == "online")
				flags |= p_web_online;
			else if (web == "idle")
				flags |= p_web_idle;
			else if (web == "dnd")
				flags |= p_web_dnd;
		}
	}

	if (j->find("status") != j->end()) {
		flags &= PF_CLEAR_STATUS;
		std::string main = string_not_null(j, "status");
		if (main == "online")
			flags |= p_status_online;
		else if (main == "idle")
			flags |= p_status_idle;
		else if (main == "dnd")
			flags |= p_status_dnd;
	}


	if (j->find("activities") != j->end()) {
		activities.clear();
		for (auto & act : (*j)["activities"]) {
			activity a;
			a.name = string_not_null(&act, "name");
			a.details = string_not_null(&act, "details");
			a.state = string_not_null(&act, "state"); // if user
			if (a.state.empty()) a.state = string_not_null(&act, "details"); // if activity from bot, maybe?
			a.type = (activity_type)int8_not_null(&act, "type");
			a.url = string_not_null(&act, "url");
			a.created_at = int64_not_null(&act, "created_at");
			if (act.find("timestamps") != act.end()) {
				a.start = int64_not_null(&(act["timestamps"]), "start");
				a.end = int64_not_null(&(act["timestamps"]), "end");
			}
			a.application_id = snowflake_not_null(&act, "application_id");
			a.flags = int8_not_null(&act, "flags");
	
			activities.push_back(a);
		}
	}

	return *this;
}

std::string presence::build_json() const {
	std::map<presence_status, std::string> status_name_mapping = {
		{ps_online, "online"},
		{ps_offline, "offline"},
		{ps_idle, "idle"},
		{ps_dnd, "dnd"}
	};
	json j({

		{"op", 3},
		{"d",	
			{
				{ "status", status_name_mapping[status()] },
				{ "since", json::value_t::null },
				{ "afk", false }
			}
		}
	});
	if (activities.size()) {
		for(const auto& i : activities){
			json j2({
				{ "name", i.name },
				{ "type", i.type }
			});
			if (!i.url.empty()) j2["url"] = i.url;
			if (!i.state.empty()) j2["details"] = i.state; // bot activity is details, not state

			j["d"]["activities"].push_back(j2);
		}
		/*j["d"]["game"] = json({ // use activities instead.
			{ "name", activities[0].name },
			{ "type", activities[0].type }
		});*/
	}

	return j.dump();
}

presence_status presence::desktop_status() const {
	return (presence_status)((flags >> PF_SHIFT_DESKTOP) & PF_STATUS_MASK);
}

presence_status presence::web_status() const {
	return (presence_status)((flags >> PF_SHIFT_WEB) & PF_STATUS_MASK);
}

presence_status presence::mobile_status() const {
	return (presence_status)((flags >> PF_SHIFT_MOBILE) & PF_STATUS_MASK);
}

presence_status presence::status() const {
	return (presence_status)((flags >> PF_SHIFT_MAIN) & PF_STATUS_MASK);
}

};
