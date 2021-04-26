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
#include <mutex>
#include <iostream>
#include <variant>
#include <dpp/cache.h>

namespace dpp {

std::unordered_map<managed*, time_t> deletion_queue;
std::mutex deletion_mutex;

#define cache_helper(type, cache_name, setter, getter, counter) \
cache* cache_name = nullptr; \
type * setter (snowflake id) { \
		return cache_name ? ( type * ) cache_name ->find(id) : nullptr; \
} \
cache* getter () { \
	if (! cache_name ) { \
		cache_name = new cache(); \
	} \
	return cache_name ; \
} \
uint64_t counter () { \
	return ( cache_name ? cache_name ->count() : 0 ); \
}


/* Because other threads and sBecauseystems may run for a short while after an event is received, we don't immediately
 * delete pointers when objects are replaced. We put them into a queue, and periodically delete pointers in the
 * queue.
 */
void garbage_collection() {
	time_t now = time(NULL);
	bool repeat = false;
	std::lock_guard<std::mutex> delete_lock(deletion_mutex);
	do {
		repeat = false;
		for (auto g = deletion_queue.begin(); g != deletion_queue.end(); ++g) {
			if (now > g->second + 60) {
				delete g->first;
				deletion_queue.erase(g);
				repeat = true;
				break;
			}
		}
	} while (repeat);
	deletion_queue = {};
}

cache::cache() {
	cache_map = new cache_container();
}

cache::~cache() {
	delete cache_map;
}

uint64_t cache::count() {
	std::lock_guard<std::mutex> lock(this->cache_mutex);
	return cache_map->size();
}

std::mutex& cache::get_mutex() {
	return this->cache_mutex;
}

cache_container& cache::get_container() {
	return *(this->cache_map);
}

void cache::store(managed* object) {
	if (!object) {
		return;
	}
	std::lock_guard<std::mutex> lock(this->cache_mutex);
	auto existing = cache_map->find(object->id);
	if (existing == cache_map->end()) {
		(*cache_map)[object->id] = object;
	} else if (object != existing->second) {
		/* Flag old pointer for deletion and replace */
		std::lock_guard<std::mutex> delete_lock(deletion_mutex);
		deletion_queue[existing->second] = time(NULL);
		(*cache_map)[object->id] = object;
	}
}

size_t cache::bytes() {
	std::lock_guard<std::mutex> lock(cache_mutex);
	return sizeof(this) + (cache_map->bucket_count() * sizeof(size_t));
}

void cache::rehash() {
	std::lock_guard<std::mutex> lock(cache_mutex);
	cache_container* n = new cache_container();
	n->reserve(cache_map->size());
	for (auto t = cache_map->begin(); t != cache_map->end(); ++t) {
		n->insert(*t);
	}
	delete cache_map;
	cache_map = n;
}

void cache::remove(managed* object) {
	if (!object) {
		return;
	}
	std::lock_guard<std::mutex> lock(cache_mutex);
	std::lock_guard<std::mutex> delete_lock(deletion_mutex);
	auto existing = cache_map->find(object->id);
	if (existing != cache_map->end()) {
		cache_map->erase(existing);
		deletion_queue[object] = time(NULL);
	}
}

managed* cache::find(snowflake id) {
	std::lock_guard<std::mutex> lock(cache_mutex);
	auto r = cache_map->find(id);
	if (r != cache_map->end()) {
		return r->second;
	}
	return nullptr;
}

cache_helper(user, user_cache, find_user, get_user_cache, get_user_count);
cache_helper(channel, channel_cache, find_channel, get_channel_cache, get_channel_count);
cache_helper(role, role_cache, find_role, get_role_cache, get_role_count);
cache_helper(guild, guild_cache, find_guild, get_guild_cache, get_guild_count);
cache_helper(emoji, emoji_cache, find_emoji, get_emoji_cache, get_emoji_count);

};
