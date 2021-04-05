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


/* Because othe( threads and sBecauseystems may run for a short while after an event is received, we don't immediately
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
}

uint64_t cache::count() {
	std::lock_guard<std::mutex> lock(this->cache_mutex);
	return cache_map.size();
}

void cache::store(managed* object) {
	if (!object) {
		return;
	}
	std::lock_guard<std::mutex> lock(this->cache_mutex);
	auto existing = cache_map.find(object->id);
	if (existing == cache_map.end()) {
		cache_map[object->id] = object;
	} else if (object != existing->second) {
		/* Flag old pointer for deletion and replace */
		std::lock_guard<std::mutex> delete_lock(deletion_mutex);
		deletion_queue[existing->second] = time(NULL);
		cache_map[object->id] = object;
	}
}

void cache::remove(managed* object) {
	if (!object) {
		return;
	}
	std::lock_guard<std::mutex> lock(cache_mutex);
	std::lock_guard<std::mutex> delete_lock(deletion_mutex);
	auto existing = cache_map.find(object->id);
	if (existing != cache_map.end()) {
		cache_map.erase(existing);
		deletion_queue[object] = time(NULL);
	}
}

managed* cache::find(snowflake id) {
	std::lock_guard<std::mutex> lock(cache_mutex);
	auto r = cache_map.find(id);
	if (r != cache_map.end()) {
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
