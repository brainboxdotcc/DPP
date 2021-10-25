
#include <dpp/discord.h>
#include <dpp/event.h>
#include <dpp/nlohmann/json.hpp>
#include <dpp/discordevents.h>
#include <dpp/stage_instance.h>

using json = nlohmann::json;

namespace dpp { namespace events {

using namespace dpp;

/**
 * @brief Handle event
 * 
 * @param client Websocket client (current shard)
 * @param j JSON data for the event
 * @param raw Raw JSON string
 */
void stage_instance_update::handle(discord_client* client, json &j, const std::string &raw) {
	if (client->creator->dispatch.stage_instance_update) {
		json& d = j["d"];
		dpp::stage_instance_update_t siu(client, raw);
		siu.id = SnowflakeNotNull(&d, "id");
		siu.channel_id = SnowflakeNotNull(&d, "channel_id");
		siu.guild_id = SnowflakeNotNull(&d, "channel_id");
		siu.privacy_level = dpp::Int8NotNull(&d, "privacy_level");
		siu.topic = StringNotNull(&d, "topic");
		client->creator->dispatch.stage_instance_update(siu);
	}
}

}};
