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

#pragma once
#include <dpp/discord.h>
#include <dpp/json_fwd.hpp>
#include <unordered_map>
#include <vector>
#include <functional>
#include <variant>

namespace dpp {

/**
 * @brief Represents a received parameter.
 * We use variant so that multiple non-related types can be contained within.
 */
typedef std::variant<std::string, dpp::role, dpp::channel, dpp::user, int32_t> command_parameter;

/**
 * @brief Parameter types when registering a command.
 * We don't pass these in when triggering the command in the handler, because it is
 * expected the developer added the command so they know what types to expect for each named
 * parameter.
 */
enum parameter_type {
	pt_string,
	pt_role,
	pt_channel,
	pt_user,
	pt_integer
};

/**
 * @brief Details of a command parameter used in registration.
 * Note that for non-slash commands optional parameters can only be at the end of
 * the list of parameters.
 */
struct command_reg_param_t {
	/**
	 * @brief Type of parameter
	 */
	parameter_type type;
	/**
	 * @brief True if the parameter is optional.
	 * For non-slash commands optional parameters may only be on the end of the list.
	 */
	bool optional;
	/**
	 * @brief Construct a new command_reg_param_t object
	 * 
	 * @param t Type of parameter
	 * @param o True if parameter is optional
	 */
	command_reg_param_t(parameter_type t, bool o);
};

/**
 * @brief Parameter list used during registration.
 * Note that use of vector/pair is important here to preserve parameter order,
 * as opposed to unordered_map (which doesnt guarantee any order at all) and 
 * std::map, which reorders keys alphabetically.
 */
typedef std::vector<std::pair<std::string, command_reg_param_t>> parameter_registration_t;

/**
 * @brief Parameter list for a called command.
 * See dpp::parameter_registration_t for an explaination as to why vector is used.
 */
typedef std::vector<std::pair<std::string, command_parameter>> parameter_list_t;

/**
 * @brief The function definition for a command handler. Expects a command name string,
 * and a list of command parameters.
 */
typedef std::function<void(const std::string&, const parameter_list_t&)> command_handler;

struct command_info_t {
	command_handler func;
	parameter_registration_t parameters;
	snowflake guild_id;
};


/**
 * @brief The commandhandler class represents a group of commands, prefixed or slash commands with handling functions.
 * 
 */
class commandhandler {
	/**
	 * @brief Commands in the handler
	 */
	std::unordered_map<std::string, command_info_t> commands;

	/**
	 * @brief Valid prefixes
	 */
	std::vector<std::string> prefixes;
public:

	/** Constructor */
	commandhandler();

	/** Destructor */
	~commandhandler();

	/**
	 * @brief Add a prefix to the command handler
	 * 
	 * @param prefix Prefix to be handled by the command handler
	 * @return commandhandler& reference to self
	 */
	commandhandler& add_prefix(const std::string &prefix);

	/**
	 * @brief Add a command to the command handler
	 * 
	 * @param command Command to be handled.
	 * Note that if any one of your prefixes is "/" this will attempt to register
	 * a global command using the API and you will receive notification of this command
	 * via an interaction event.
	 * 
	 * @param handler Handler function
	 * @param parameters Parameters to use for the command
	 * @return commandhandler& reference to self
	 */
	commandhandler& add_command(const std::string &command, const parameter_registration_t &parameters, command_handler handler, snowflake guild_id = 0);

	/**
	 * @brief Route a command from the on_message_create function.
	 * Call this method from within your on_message_create with the received
	 * dpp::message object.
	 * 
	 * @param msg message to parse
	 */
	void route(const class dpp::message& msg);

	/**
	 * @brief Route a command from the on_interaction_create function.
	 * Call this method from your on_interaction_create with the recieved
	 * dpp::command_interaction object.
	 * 
	 * @param cmd command interaction to parse
	 */
	void route(const class dpp::command_interaction& cmd);

	/**
	 * @brief Reply to a message.
	 * You should use this method rather than cluster::message_create as
	 * the way you reply varies between slash commands and message commands.
	 * 
	 * @param m message to reply with.
	 */
	void reply(const dpp::message &m);
};

};