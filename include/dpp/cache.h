#pragma once

#include <dpp/discord.h>
#include <map>
#include <mutex>

namespace dpp {

	class cache {
	private:
		std::mutex cache_mutex;
		std::unordered_map<uint64_t, managed*> cache_map;
	public:
		void store(managed* object);
		void remove(managed* object);
		managed* find(snowflake id);
		uint64_t count();
	};

	void garbage_collection();

	#define cache_decl(type, setter, getter, counter) type * setter (snowflake id); cache * getter (); uint64_t counter ();
	cache_decl(user, find_user, get_user_cache, get_user_count);
	cache_decl(guild, find_guild, get_guild_cache, get_guild_count);
	cache_decl(role, find_role, get_role_cache, get_role_count);
	cache_decl(channel, find_channel, get_channel_cache, get_channel_count);
};

