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
	};

	void garbage_collection();

	#define cache_decl(type, setter, getter) type * setter (snowflake id); cache * getter ();
	cache_decl(user, find_user, get_user_cache);
	cache_decl(guild, find_guild, get_guild_cache);
	cache_decl(role, find_role, get_role_cache);
	cache_decl(channel, find_channel, get_channel_cache);
};

