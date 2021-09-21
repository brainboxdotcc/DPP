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
#include <dpp/fmt/format.h>
#include <sstream>

namespace dpp {

param_info::param_info(parameter_type t, bool o, const std::string &d, const std::map<std::string, std::string> &opts) : type(t), optional(o), description(d), choices(opts)
{
}

commandhandler::commandhandler(cluster* o, bool auto_hook_events, snowflake application_id) : slash_commands_enabled(false), owner(o), app_id(application_id)
{
	if (!application_id && o->me.id) {
		app_id = o->me.id;
	}
	if (auto_hook_events) {
		o->on_interaction_create([this](const dpp::interaction_create_t &event) {
			this->route(event);
		});
		o->on_message_create([this](const dpp::message_create_t & event) {
			this->route(*event.msg);
		});
	}

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
		if (this->app_id == 0) {
			if (owner->me.id == 0) {
				throw dpp::exception("Command handler not ready (i don't know my application ID)");
			} else {
				this->app_id = owner->me.id;
			}
		}
		dpp::slashcommand newcommand;
		/* Create a new global command on ready event */
		newcommand.set_name(lowercase(command)).set_description(description).set_application_id(this->app_id);

		for (auto& parameter : parameters) {
			command_option_type cot;
			switch (parameter.second.type) {
				case pt_boolean:
					cot = co_boolean;
				break;
				case pt_integer:
					cot = co_integer;
				break;
				case pt_string:
					cot = co_string;
				break;
				case pt_user:
					cot = co_user;
				break;
				case pt_role:
					cot = co_role;
				break;
				case pt_channel:
					cot = co_channel;
				break;
			}

			command_option opt(cot, parameter.first, parameter.second.description, !parameter.second.optional);
			if (!parameter.second.choices.empty()) {
				for (auto& c : parameter.second.choices) {
					opt.add_choice(dpp::command_option_choice(c.second, c.first));
				}
			}
			newcommand.add_option(opt);
		}
		/* Register the command */
		if (guild_id) {
			owner->guild_command_create(newcommand, guild_id, [command, this](const dpp::confirmation_callback_t &callback) {
				if (callback.is_error()) {
					this->owner->log(dpp::ll_error, fmt::format("Failed to register guild slash command '{}': {}", command, callback.http_info.body));
				}
			});
		} else {
			owner->global_command_create(newcommand, [command, this](const dpp::confirmation_callback_t &callback) {
				if (callback.is_error()) {
					this->owner->log(dpp::ll_error, fmt::format("Failed to register global slash command '{}': {}", command, callback.http_info.body));
				}
			});
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

/* Note that message based command routing relies on cache to resolve ping types (e.g. user, channel ping).
 * There isn't really a way around this for many things because there is no 'resolved' member for it.
 * We only get resolved information for the user issuing the command.
 */
void commandhandler::route(const class dpp::message& msg)
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
						ss >> x;
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
						ss >> x;
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
						ss >> x;
						if (x.length() > 4 && x[0] == '<' && x[1] == '@') {
							snowflake uid = from_string<uint64_t>(x.substr(2, x.length() - 1), std::dec);
							user* u = dpp::find_user(uid);
							if (u) {
								dpp::resolved_user m;
								m.user = *u;
								dpp::guild* g = dpp::find_guild(msg.guild_id);
								if (g->members.find(uid) != g->members.end()) {
									m.member = g->members[uid];
								}
								param = m;
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
			command_source source;
			source.command_id = 0;
			source.guild_id = msg.guild_id;
			source.channel_id = msg.channel_id;
			source.issuer = msg.author;
			found_cmd->second.func(command, call_params, source);
		}
	}
}

void commandhandler::route(const class interaction_create_t & event)
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
			dpp::command_resolved res = event.command.resolved;

			if (p.second.optional && slash_parameter.index() == 0 /* std::monostate */) {
				/* Missing optional parameter, skip this */
				continue;
			}
			
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
						/* Use cache if the role is in the cache */
						param = *r;
					} else {
						/* Otherwise use interaction resolved fields */
						if (res.roles.find(rid) != res.roles.end()) {
							param = res.roles[rid];
						}
					}
				}
				break;
				case pt_channel: {
					snowflake cid = std::get<snowflake>(slash_parameter);
					channel* c = dpp::find_channel(cid);
					if (c) {
						/* Use cache if the channel is in the cache */
						param = *c;
					} else {
						/* Otherwise use interaction resolved fields */
						if (res.channels.find(cid) != res.channels.end()) {
							param = res.channels[cid];
						}
					}
				}
				break;
				case pt_user: {
					snowflake uid = std::get<snowflake>(slash_parameter);
					/* TODO: Make this used resolved, not cache */
					user* u = dpp::find_user(uid);
					if (u) {
						/* Use the cache if the user is in the cache */
						dpp::resolved_user m;
						m.user = *u;
						dpp::guild* g = dpp::find_guild(event.command.guild_id);
						if (g->members.find(uid) != g->members.end()) {
							m.member = g->members[uid];
						}
						param = m;
					} else {
						/* Otherwise use interaction resolved fields */
						if (
							event.command.resolved.users.find(uid) != event.command.resolved.users.end()
							&&
							event.command.resolved.members.find(uid) != event.command.resolved.members.end()
						) {
							/* Fill in both member and user info */
							dpp::resolved_user m;
							m.member = res.members[uid];
							m.user = res.users[uid];
							param = m;
						}
					}
				}
				break;
				case pt_integer: {
					int32_t i = std::get<int32_t>(slash_parameter);
					param = i;
				}
				break;
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
		command_source source;
		source.command_id = event.command.id;
		source.command_token = event.command.token;
		source.guild_id = event.command.guild_id;
		source.channel_id = event.command.channel_id;
		source.issuer = (user*)&event.command.usr;
		found_cmd->second.func(cmd.name, call_params, source);
	}
}

void commandhandler::reply(const dpp::message &m, command_source source)
{
	dpp::message msg = m;
	msg.guild_id = source.guild_id;
	msg.channel_id = source.channel_id;
	if (!source.command_token.empty() && source.command_id) {
		owner->interaction_response_create(source.command_id, source.command_token, dpp::interaction_response(ir_channel_message_with_source, msg), [this](const dpp::confirmation_callback_t &callback) {
			if (callback.is_error()) {
				this->owner->log(dpp::ll_error, fmt::format("Failed to send interaction response: {}", callback.http_info.body));
			}
		});
	} else {
		owner->message_create(msg);
	}
}

void commandhandler::thinking(command_source source)
{
	dpp::message msg;
	msg.content = "*";
	msg.guild_id = source.guild_id;
	msg.channel_id = source.channel_id;
	if (!source.command_token.empty() && source.command_id) {
		owner->interaction_response_create(source.command_id, source.command_token, dpp::interaction_response(ir_deferred_channel_message_with_source, msg));
	}
}

void commandhandler::thonk(command_source source)
{
	thinking(source);
}

};