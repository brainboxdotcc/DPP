
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
		siu.updated.fill_from_json(&j);
		client->creator->dispatch.stage_instance_update(siu);
	}
}

}};
