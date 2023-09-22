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
#include <dpp/export.h>
#include <mutex>
#include <iostream>
#include <variant>
#include <dpp/cache.h>
#include <dpp/exception.h>

namespace dpp {

std::unordered_map<std::type_index, cache_registry*> regs;
std::unordered_map<managed*, time_t> deletion_queue;
std::mutex deletion_mutex;

#define cache_helper(type, cache_name, setter, getter, counter) \
type * setter (snowflake id) { \
		return ( type * ) cache_registry::get_cache<type>()->find(id); \
} \
cache<type>* getter () { \
	return cache_registry::get_cache<type>(); \
} \
uint64_t counter () { \
	return cache_registry::get_cache<type>()->count() ; \
}

cache_helper(user, user_cache, find_user, get_user_cache, get_user_count);
cache_helper(channel, channel_cache, find_channel, get_channel_cache, get_channel_count);
cache_helper(role, role_cache, find_role, get_role_cache, get_role_count);
cache_helper(guild, guild_cache, find_guild, get_guild_cache, get_guild_count);
cache_helper(emoji, emoji_cache, find_emoji, get_emoji_cache, get_emoji_count);

} // namespace dpp
