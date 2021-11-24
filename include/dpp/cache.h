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
#include <dpp/export.h>
#include <dpp/discord.h>
#include <map>
#include <mutex>
#include <shared_mutex>

namespace dpp {

extern std::unordered_map<managed*, time_t> deletion_queue;
extern std::mutex deletion_mutex;

/** forward declaration */
class guild_member;

/**
 * @brief A cache object maintains a cache of dpp::managed objects.
 * This is for example users, channels or guilds.
 */
template<class T> class cache {
private:
	/** Mutex to protect the cache */
	std::shared_mutex cache_mutex;

	/** Cached items */
	std::unordered_map<snowflake, T*>* cache_map;
public:

	/**
	 * @brief Construct a new cache object
	 */
	cache() {
		cache_map = new std::unordered_map<snowflake, T*>;
	}

	/**
	 * @brief Destroy the cache object
	 */
	~cache() {
		std::unique_lock l(cache_mutex);
		delete cache_map;			
	}

	/**
	 * @brief Store an object in the cache.
	 * 
	 * @param object object to store
	 */
	void store(T* object) {
		if (!object) {
			return;
		}
		std::unique_lock l(cache_mutex);
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

	/**
	 * @brief Remove an object from the cache.
	 * 
	 * @param object object to remove
	 */
	void remove(T* object) {
		if (!object) {
			return;
		}
		std::unique_lock l(cache_mutex);
		std::lock_guard<std::mutex> delete_lock(deletion_mutex);
		auto existing = cache_map->find(object->id);
		if (existing != cache_map->end()) {
			cache_map->erase(existing);
			deletion_queue[object] = time(NULL);
		}
	}

	/**
	 * @brief Find an object in the cache by id.
	 * 
	 * @param id Object id to find
	 * @return Found object or nullptr if not found
	 */
	T* find(snowflake id) {
		std::shared_lock l(cache_mutex);
		auto r = cache_map->find(id);
		if (r != cache_map->end()) {
			return r->second;
		}
		return nullptr;
	}

	/**
	 * @brief Return a count of the number of items in the cache.
	 * 
	 * @return uint64_t count of items in the cache
	 */
	uint64_t count() {
		std::shared_lock l(cache_mutex);
		return cache_map->size();
	}

	/** 
	 * @brief Return the cache's locking mutex. Use this whenever
	 * you manipulate or iterate raw elements in the cache!
	 * 
	 * @return The mutex used to protect the container
	 */
	std::shared_mutex& get_mutex() {
		return this->cache_mutex;			
	}

	/**
	 * @brief Get the container map
	 * @warning Be sure to use cache::get_mutex() correctly if you
	 * manipulate or iterate the map returned by this method! If you do
	 * not, this is not thread safe and will cause crashes!
	 * @see cache::get_mutex
	 * 
	 * @return A reference to the cache's container map
	 */
	auto & get_container() {
		return *(this->cache_map);		
	}

	/**
	 * @brief "Rehash" a cache by cleaning out used RAM
	 * @warning May be time consuming!
	 */
	void rehash() {
		std::unique_lock l(cache_mutex);
		std::unordered_map<snowflake, T*>* n = new std::unordered_map<snowflake, T*>;
		n->reserve(cache_map->size());
		for (auto t = cache_map->begin(); t != cache_map->end(); ++t) {
			n->insert(*t);
		}
		delete cache_map;
		cache_map = n;
	}

	/**
	 * @brief Get "real" size in RAM of the cache
	 * 
	 * @return size_t 
	 */
	size_t bytes() {
		std::shared_lock l(cache_mutex);
		return sizeof(this) + (cache_map->bucket_count() * sizeof(size_t));
	}

};

/** Run garbage collection across all caches removing deleted items
 * that have been deleted over 60 seconds ago.
 */
void DPP_EXPORT garbage_collection();

#define cache_decl(type, setter, getter, counter) DPP_EXPORT type * setter (snowflake id); DPP_EXPORT cache<type> * getter ();  DPP_EXPORT uint64_t counter ();

/* Declare major caches */
cache_decl(user, find_user, get_user_cache, get_user_count);
cache_decl(guild, find_guild, get_guild_cache, get_guild_count);
cache_decl(role, find_role, get_role_cache, get_role_count);
cache_decl(channel, find_channel, get_channel_cache, get_channel_count);
cache_decl(emoji, find_emoji, get_emoji_cache, get_emoji_count);

/**
 * @brief Get the guild_member from cache of given IDs
 *
 * @param guild_id ID of the guild to find guild_member for
 * @param user_id ID of the user to find guild_member for
 *
 * @throw dpp::cache_exception if the guild or guild_member is not found in the cache
 * @return guild_member the cached object, if found
 */
guild_member find_guild_member(const snowflake guild_id, const snowflake user_id);

};

