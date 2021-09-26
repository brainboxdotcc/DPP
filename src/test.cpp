#undef DPP_BUILD

#include <dpp/dpp.h>
#include <iostream>
#include <sstream>
#include <dpp/nlohmann/json.hpp>
#include <dpp/fmt/format.h>

using json = nlohmann::json;

int main()
{
	json configdocument;
	std::ifstream configfile("../config.json");
	configfile >> configdocument;
	dpp::cluster bot(configdocument["token"]);


	bot.on_message_create([&bot](const dpp::message_create_t & event) {
		std::stringstream ss(event.msg->content);
		std::string cmd;
		ss >> cmd;
		if (cmd == "dovoice") {
			dpp::snowflake s;
			ss >> s;
			event.from->connect_voice(event.msg->guild_id, s);
		}
	});

	bot.on_ready([&](const dpp::ready_t & event) {
	});



	bot.on_voice_ready([&](const dpp::voice_ready_t& ev) {
		bot.log(dpp::ll_critical, std::to_string(ev.voice_channel_id) + " vcid");
		bot.log(dpp::ll_critical, std::to_string(ev.voice_client->channel_id) +" vc cid");
		bot.log(dpp::ll_critical, std::to_string(ev.voice_client->server_id)+" vc sid");
	});
	
	bot.on_log([](const dpp::log_t & event) {
		if (event.severity > dpp::ll_trace) {
			std::cout << event.message << "\n";
		}
	});

	bot.log(dpp::ll_debug, fmt::format("Voice support: {}", dpp::utility::has_voice()));

	bot.start(false);
	return 0;
}
