#include <dpp/discord.h>
#include <mutex>

namespace dpp {

user_map users;
channel_map channels;
guild_map guilds;
std::mutex user_mutex;
std::mutex channel_mutex;
std::mutex guild_mutex;

void store_guild(guild* g) {
	std::lock_guard<std::mutex> lock(guild_mutex);
	if (guilds.find(g->id) != guilds.end()) {
		guilds[g->id] = g;
	}
}

guild* find_guild(snowflake id) {
	std::lock_guard<std::mutex> lock(guild_mutex);
	auto r = guilds.find(id);
	if (r != guilds.end()) {
		return r->second;
	}
	return nullptr;
}

void store_user(user * u) {
	std::lock_guard<std::mutex> lock(user_mutex);
	if (users.find(u->id) != users.end()) {
		users[u->id] = u;
	}
}

user* find_user(snowflake id) {
	std::lock_guard<std::mutex> lock(user_mutex);
	auto r = users.find(id);
	if (r != users.end()) {
		return r->second;
	}
	return nullptr;
}

void store_channel(channel* c) {
	std::lock_guard<std::mutex> lock(channel_mutex);
	if (channels.find(c->id) != channels.end()) {
		channels[c->id] = c;
	}
}

channel* find_channel(snowflake id) {
	std::lock_guard<std::mutex> lock(channel_mutex);
	auto r = channels.find(id);
	if (r != channels.end()) {
		return r->second;
	}
	return nullptr;
}

};
