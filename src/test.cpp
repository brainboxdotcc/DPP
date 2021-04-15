#include <dpp/dpp.h>
#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

int main(int argc, char const *argv[])
{
	srand(time(NULL));

	std::ifstream input ("../testdata/Robot.pcm", std::ios::in|std::ios::binary|std::ios::ate);
	uint8_t* noise = nullptr;
	size_t noise_size = 0;

	if (input.is_open()) {
		noise_size = input.tellg();
		noise = new uint8_t[noise_size];
		input.seekg (0, std::ios::beg);
		input.read ((char*)noise, noise_size);
		input.close();
	}

	json configdocument;
	std::ifstream configfile("../config.json");
	configfile >> configdocument;

	dpp::cluster bot(configdocument["token"], dpp::i_default_intents | dpp::i_guild_members);

	bot.on_log([&bot](const dpp::log_t & event) {
		if (event.severity >= dpp::ll_debug) {
			std::cout << dpp::utility::current_date_time() << " [" << dpp::utility::loglevel(event.severity) << "] " << event.message << "\n";
		}
	});

	bot.on_voice_ready([&bot](const dpp::voice_ready_t & event) {
		uint16_t beep[131057];
		for (int x = 0; x < 131057; ++x) {
			beep[x] = rand() % 65535;
		}
		event.voice_client->SendAudio(beep, sizeof(beep));
	});
	
	bot.on_voice_buffer_send([&bot](const dpp::voice_buffer_send_t & event) {
	});

	/* Attach to the message_create event to get notified of new messages */
	bot.on_message_create([&bot, noise, noise_size](const dpp::message_create_t & event) {

		std::stringstream ss(event.msg->content);
		std::string command;
		ss >> command;

		if (command == ".voicetest") {
			dpp::snowflake channel_id;
			ss >> channel_id;
			bot.log(dpp::ll_info, fmt::format("Starting voice test! Channel: {} Guild: {}", channel_id, event.msg->guild_id));
			if (channel_id) {
				dpp::DiscordClient* dc = bot.get_shard(0);
				dc->ConnectVoice(event.msg->guild_id, channel_id);
			}
		}
		if (command == ".noise") {
			/* This assumes that there is one shard and the voice channel's guild is is on it. 
			 * Only for testing purposes, DO NOT make this assumption in the real world!
			 */
			dpp::DiscordClient* dc = bot.get_shard(0);
			dpp::voiceconn* v = dc->GetVoice(event.msg->guild_id);
			if (v && v->voiceclient && v->voiceclient->IsReady()) {
				v->voiceclient->SendAudio((uint16_t*)noise, noise_size);
			}

		}
	});

	/* This method call actually starts the bot by connecting all shards in the cluster */
	bot.start(false);
	
	/* Never reached */
	return 0;
}

