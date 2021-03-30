#include <map>
#include <dpp/discord.h>
#include <dpp/cluster.h>
#include <dpp/discordclient.h>
#include <spdlog/spdlog.h>

namespace dpp {

void cluster::start(const std::string &token, uint32_t intents, uint32_t shards, uint32_t cluster_id, uint32_t maxclusters, spdlog::logger* log) {
	/* Start up all shards */
	for (uint32_t s = 0; s < shards; ++s) {
		/* Filter out shards that arent part of the current cluster, if the bot is clustered */
		if (s % maxclusters == cluster_id) {
			/* TODO: DiscordClient should spawn a thread in its Run() */
			this->shards[s] = new DiscordClient(this, s, shards, token, intents, log);
			this->shards[s]->Run();
			::sleep(5);
		}
	}
}

};
