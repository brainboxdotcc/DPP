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
#include <dpp/stringops.h>
#include <dpp/nlohmann/json.hpp>

namespace dpp {

commandhandler::commandhandler()
{
}

commandhandler::~commandhandler()
{
}

commandhandler& commandhandler::add_prefix(const std::string &prefix)
{
	return *this;
}

commandhandler& commandhandler::add_command(const std::string &command, command_handler handler)
{
	return *this;
}

void commandhandler::route(const dpp::message& msg)
{
}

void commandhandler::route(const dpp::command_interaction& cmd)
{
}

void commandhandler::reply(const dpp::message &m)
{
}

};