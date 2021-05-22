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
#include <dpp/slashcommand.h>
#include <dpp/discordevents.h>
#include <dpp/discord.h>
#include <dpp/nlohmann/json.hpp>
#include <iostream>

namespace dpp {

using json = nlohmann::json;

slashcommand::slashcommand() : managed()
{
}

slashcommand::~slashcommand() {
}

slashcommand& slashcommand::fill_from_json(nlohmann::json* j) {
	id = SnowflakeNotNull(j, "id");
	return *this;
}

void to_json(json& j, const command_option_choice& choice) {
	j["name"] = choice.name;
	if (std::holds_alternative<uint32_t>(choice.value)) {
		j["value"] = std::get<uint32_t>(choice.value);
	} else if (std::holds_alternative<bool>(choice.value)) {
		j["value"] = std::get<bool>(choice.value);
	} else if (std::holds_alternative<snowflake>(choice.value)) {
		j["value"] = std::to_string(std::get<uint64_t>(choice.value));
	} else {
		j["value"] = std::get<std::string>(choice.value);
	}
}

void to_json(json& j, const command_option& opt) {
	j["name"] = opt.name;
	j["description"] = opt.description;
	j["type"] = opt.type;
	j["required"] = opt.required;

	if (opt.options.size()) {
		j["options"] = json();

		for (const auto& opt : opt.options) {
			json jopt = opt;
			j["options"].push_back(jopt);
		}
	}

	if (opt.choices.size()) {
		j["choices"] = json();

		for (const auto& choice : opt.choices) {
			json jchoice = choice;
			j["choices"].push_back(jchoice);
		}
	}
}

void to_json(json& j, const slashcommand& p) {
	j["name"] = p.name;
	j["description"] = p.description;

	if (p.options.size()) {
		j["options"] = json();

		for (const auto& opt : p.options) {
			json jopt = opt;
			j["options"].push_back(jopt);
		}
	}
}

std::string slashcommand::build_json(bool with_id) const {
	json j = *this;

	if (with_id) {
		j["id"] = std::to_string(id);
	}

	return j.dump();
}

slashcommand& slashcommand::set_name(const std::string &n) {
	name = n;
	return *this;
}

slashcommand& slashcommand::set_description(const std::string &d) {
	description = d;
	return *this;
}

slashcommand& slashcommand::set_application_id(snowflake i) {
	application_id = i;
	return *this;
}

command_option_choice::command_option_choice(const std::string &n, command_value v) : name(n), value(v)
{
}

command_option::command_option(command_option_type t, const std::string &n, const std::string &d, bool r) :
	type(t), name(n), description(d), required(r)
{
}

command_option& command_option::add_choice(const command_option_choice &o)
{
	choices.push_back(o);
	return *this;
}

command_option& command_option::add_option(const command_option &o)
{
	options.push_back(o);
	return *this;
}

slashcommand& slashcommand::add_option(const command_option &o)
{
	options.push_back(o);
	return *this;
}

interaction& interaction::fill_from_json(nlohmann::json* j) {
	j->get_to(*this);
	return *this;
}

std::string interaction::build_json(bool with_id) const {
	return "";
}

void from_json(const nlohmann::json& j, command_data_option& cdo) {
	cdo.name = StringNotNull(&j, "name");
	cdo.type = (command_option_type)Int8NotNull(&j, "type");

	if (j.contains("options") && !j.at("options").is_null()) {
		j.at("options").get_to(cdo.options);
	}

	if (j.contains("value") && !j.at("value").is_null()) {
		switch (cdo.type) {
		case co_boolean:
			cdo.value = j.at("value").get<bool>();
			break;
		case co_channel:
		case co_role:
		case co_user:
			cdo.value = SnowflakeNotNull(&j, "value");
			break;
		case co_integer:
			cdo.value = j.at("value").get<uint32_t>();
			break;
		case co_string:
			cdo.value = j.at("value").get<std::string>();
			break;
		}
	}
}

void from_json(const nlohmann::json& j, command_interaction& ci) {
	ci.id = SnowflakeNotNull(&j, "id");
	ci.name = StringNotNull(&j, "name");

	if (j.contains("options") && !j.at("options").is_null()) {
		j.at("options").get_to(ci.options);
	}
}

void from_json(const nlohmann::json& j, button_interaction& bi) {
	bi.component_type = Int8NotNull(&j, "component_type");
	bi.custom_id = StringNotNull(&j, "custom_id");
}

void from_json(const nlohmann::json& j, interaction& i) {
	i.id = SnowflakeNotNull(&j, "id");
	i.application_id = SnowflakeNotNull(&j, "application_id");
	i.channel_id = SnowflakeNotNull(&j, "channel_id");
	i.guild_id = SnowflakeNotNull(&j, "guild_id");

	i.type = Int8NotNull(&j, "type");
	i.token = StringNotNull(&j, "token");
	i.version = Int8NotNull(&j, "version");

	if (j.contains("member") && !j.at("member").is_null()) {
		j.at("member").get_to(i.member);
		if (j.at("member").contains("user") && !j.at("member").at("user").is_null()) {
			j.at("member").at("user").get_to(i.usr);
		}
	}

	if (j.contains("data") && !j.at("data").is_null()) {
		if (i.type == it_application_command) {
			command_interaction ci;
			j.at("data").get_to(ci);
			i.data = ci;
		} else if (i.type == it_component_button) {
			button_interaction bi;
			j.at("data").get_to(bi);
			i.data = bi;
		}
	}
}

interaction_response::interaction_response() {
	msg = new message();
}

interaction_response::~interaction_response() {
	delete msg;
}

interaction_response::interaction_response(interaction_response_type t, const message& m) : interaction_response() {
	type = t;
	*msg = m;
}

interaction_response& interaction_response::fill_from_json(nlohmann::json* j) {
	type = (interaction_response_type)Int8NotNull(j, "type");
	if (j->find("data") != j->end()) {
		msg->fill_from_json(&((*j)["data"]));
	}
	return *this;
}

std::string interaction_response::build_json() const {
	json j;
	json msg_json = json::parse(msg->build_json());
	j["type"] = this->type;
	auto cid = msg_json.find("channel_id");
	if (cid != msg_json.end()) {
		msg_json.erase(cid);
	}
	j["data"] = msg_json;
	return j.dump();
}

};
