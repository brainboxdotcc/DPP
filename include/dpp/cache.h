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

namespace dpp {

	/**
	 * @brief A set of cached managed objects
	 */
	typedef std::unordered_map<uint64_t, managed*> cache_container;

	/**
	 * @brief A cache object maintains a cache of dpp::managed objects.
	 * This is for example users, channels or guilds.
	 */
	class DPP_EXPORT cache {
	private:

		/** Mutex to protect the cache */
		std::mutex cache_mutex;

		/** Cached items */
		cache_container* cache_map;

	public:

		/**
		 * @brief Construct a new cache object
		 */
		cache();

		/**
		 * @brief Destroy the cache object
		 */
		~cache();

		/** Store an object in the cache.
		 * @param object object to store
		 */
		void store(managed* object);

		/** Remove an object from the cache.
		 * @param object object to remove
		 */
		void remove(managed* object);

		/** Find an object in the cache by id.
		 * @param id Object id to find
		 */
		managed* find(snowflake id);

		/** Return a count of the number of items in the cache.
		 */
		uint64_t count();

		/** 
		 * @brief Return the cache's locking mutex. Use this whenever
		 * you manipulate or iterate raw elements in the cache!
		 * 
		 * @return The mutex used to protect the container
		 */
		std::mutex& get_mutex();

		/**
		 * @brief Get the container map
		 * @warning Be sure to use cache::get_mutex() correctly if you
		 * manipulate or iterate the map returned by this method! If you do
		 * not, this is not thread safe and will cause crashes!
		 * @see cache::get_mutex
		 * 
		 * @return cache_container& A reference to the cache's container map
		 */
		cache_container& get_container();

		/**
		 * @brief "Rehash" a cache by cleaning out used RAM
		 * @warning May be time consuming!
		 */
		void rehash();

		/**
		 * @brief Get "real" size in RAM of the cache
		 * 
		 * @return size_t 
		 */
		size_t bytes();

	};

	/** Run garbage collection across all caches removing deleted items
	 * that have been deleted over 60 seconds ago.
	 */
	void DPP_EXPORT garbage_collection();

	#define cache_decl(type, setter, getter, counter) DPP_EXPORT type * setter (snowflake id); DPP_EXPORT cache * getter ();  DPP_EXPORT uint64_t counter ();

	/* Declare major caches */
	cache_decl(user, find_user, get_user_cache, get_user_count);
	cache_decl(guild, find_guild, get_guild_cache, get_guild_count);
	cache_decl(role, find_role, get_role_cache, get_role_count);
	cache_decl(channel, find_channel, get_channel_cache, get_channel_count);
	cache_decl(emoji, find_emoji, get_emoji_cache, get_emoji_count);
};

