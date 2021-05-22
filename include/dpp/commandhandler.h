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
#include <map>

namespace dpp {

typedef std::function<void(const std::string&, std::map<std::string, std::string>)> command_handler;

/**
 * @brief The commandhandler class represents a group of commands, prefixed or slash commands with handling functions.
 * 
 */
class commandhandler {
	/**
	 * @brief Commands in the handler
	 * 
	 */
	std::map<std::string, command_handler> commands;

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
	 * @param handler Handler function
	 * @return commandhandler& reference to self
	 */
	commandhandler& add_command(const std::string &command, command_handler handler);

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
	 * Call this method from yuor on_interaction_create with the recieved
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