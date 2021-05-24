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
#include <dpp/commandhandler.h>
#include <dpp/cache.h>
#include <dpp/cluster.h>
#include <dpp/dispatcher.h>
#include <dpp/stringops.h>
#include <dpp/nlohmann/json.hpp>
#include <sstream>

namespace dpp {

command_reg_param_t::command_reg_param_t(parameter_type t, bool o, const std::string &d, const std::map<std::string, std::string> &opts) : type(t), optional(o), description(d), choices(opts)
{
}

commandhandler::commandhandler(cluster* o) : slash_commands_enabled(false), owner(o)
{
}

commandhandler& commandhandler::set_owner(cluster* o)
{
	owner = o;
	return *this;
}

commandhandler::~commandhandler()
{
}

commandhandler& commandhandler::add_prefix(const std::string &prefix)
{
	prefixes.push_back(prefix);
	if (prefix == "/") {
		if (!slash_commands_enabled) {
			/* Register existing slash commands */
			slash_commands_enabled = true;
		} else {
			slash_commands_enabled = true;
		}
	}
	return *this;
}

commandhandler& commandhandler::add_command(const std::string &command, const parameter_registration_t &parameters, command_handler handler, const std::string &description, snowflake guild_id)
{
	command_info_t i;
	i.func = handler;
	i.guild_id = guild_id;
	i.parameters = parameters;
	commands[lowercase(command)] = i;
	if (slash_commands_enabled) {
		dpp::slashcommand newcommand;
		/* Create a new global command on ready event */
		newcommand.set_name(lowercase(command)).set_description(description).set_application_id(owner->me.id);

		for (auto& parameter : parameters) {
			/* TODO: Add support for other types! */
			dpp::command_option opt(dpp::co_string, parameter.first, parameter.second.description, !parameter.second.optional);
			if (!parameter.second.choices.empty()) {
				for (auto& c : parameter.second.choices) {
					opt.add_choice(dpp::command_option_choice(c.second, c.first));
				}
			}
			newcommand.add_option(opt);
		}
		/* Register the command */
		if (guild_id) {
			owner->guild_command_create(newcommand, guild_id);
		} else {
			owner->global_command_create(newcommand);
		}
	}
	return *this;
}

bool commandhandler::string_has_prefix(std::string &str)
{
	size_t str_length = utility::utf8len(str);
	for (auto& p : prefixes) {
		size_t prefix_length = utility::utf8len(p);
		if (utility::utf8substr(str, 0, prefix_length) == p) {
			str.erase(str.begin(), str.begin() + prefix_length);
			return true;
		}
	}
	return false;
}

void commandhandler::route(const dpp::message& msg)
{
	std::string msg_content = msg.content;
	if (string_has_prefix(msg_content)) {
		/* Put the string into stringstream to parse parameters at spaces.
		 * We use stringstream as it handles multiple spaces etc nicely.
		 */
		std::stringstream ss(msg_content);
		std::string command;
		ss >> command;
		/* Prefixed command, the prefix was removed */
		auto found_cmd = commands.find(lowercase(command));
		if (found_cmd != commands.end()) {
			/* Filter out guild specific commands that are not for the current guild */
			if (found_cmd->second.guild_id && found_cmd->second.guild_id != msg.guild_id) {
				return;
			}

			parameter_list_t call_params;

			/* Command found; parse parameters */
			for (auto& p : found_cmd->second.parameters) {
				command_parameter param;

				/* Check for end of stream */
				if (!ss) {
					/* If it's an optional param, we dont care */
					if (!p.second.optional) {
						/* Trigger missing parameter handler? */
					}
					break;
				}

				switch (p.second.type) {
					case pt_string: {
						std::string x;
						ss >> x;
						param = x;
					}
					break;
					case pt_role: {
						std::string x;
						if (x.length() > 4 && x[0] == '<' && x[1] == '&') {
							snowflake rid = from_string<uint64_t>(x.substr(2, x.length() - 1), std::dec);
							role* r = dpp::find_role(rid);
							if (r) {
								param = *r;
							}
						}
					}
					break;
					case pt_channel: {
						std::string x;
						if (x.length() > 4 && x[0] == '<' && x[1] == '#') {
							snowflake cid = from_string<uint64_t>(x.substr(2, x.length() - 1), std::dec);
							channel* c = dpp::find_channel(cid);
							if (c) {
								param = *c;
							}
						}
					}
					break;
					case pt_user: {
						std::string x;
						if (x.length() > 4 && x[0] == '<' && x[1] == '@') {
							snowflake uid = from_string<uint64_t>(x.substr(2, x.length() - 1), std::dec);
							user* u = dpp::find_user(uid);
							if (u) {
								param = *u;
							}
						}
					}
					break;
					case pt_integer: {
						int32_t x = 0;
						ss >> x;
						param = x;
					}
					case pt_boolean: {
						std::string x;
						bool y = false;
						ss >> x;
						x = lowercase(x);
						if (x == "yes" || x == "1" || x == "true") {
							y = true;
						}
						param = y;
					}
					break;
				}

				/* Add parameter to the list */
				call_params.push_back(std::make_pair(p.first, param));
			}

			/* Call command handler */
			found_cmd->second.func(command, call_params);
		}
	}
}

void commandhandler::route(const interaction_create_t & event)
{
	/* We don't need to check for prefixes here, slash command interactions
	 * dont have prefixes at all.
	 */
	command_interaction cmd = std::get<command_interaction>(event.command.data);
	auto found_cmd = commands.find(lowercase(cmd.name));
	if (found_cmd != commands.end()) {
		/* Command found; parse parameters */
		parameter_list_t call_params;
		for (auto& p : found_cmd->second.parameters) {
			command_parameter param;
			const command_value& slash_parameter = event.get_parameter(p.first);
			
			switch (p.second.type) {
				case pt_string: {
					std::string s = std::get<std::string>(slash_parameter);
					param = s;
				}
				break;
				case pt_role: {
					snowflake rid = std::get<snowflake>(slash_parameter);
					role* r = dpp::find_role(rid);
					if (r) {
						param = *r;
					}
				}
				break;
				case pt_channel: {
					snowflake cid = std::get<snowflake>(slash_parameter);
					channel* c = dpp::find_channel(cid);
					if (c) {
						param = *c;
					}
				}
				break;
				case pt_user: {
					snowflake uid = std::get<snowflake>(slash_parameter);
					user* u = dpp::find_user(uid);
					if (u) {
						param = *u;
					}
				}
				break;
				case pt_integer: {
					int32_t i = std::get<int32_t>(slash_parameter);
					param = i;
				}
				case pt_boolean: {
					bool b = std::get<bool>(slash_parameter);
					param = b;
				}
				break;
			}

			/* Add parameter to the list */
			call_params.push_back(std::make_pair(p.first, param));
		}

		/* Call command handler */
		found_cmd->second.func(cmd.name, call_params);
	}
}

void commandhandler::reply(const dpp::message &m)
{
}

};