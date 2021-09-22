#undef DPP_BUILD

#include <dpp/dpp.h>
#include <iostream>
#include <dpp/nlohmann/json.hpp>
#include <dpp/fmt/format.h>

using json = nlohmann::json;

int main()
{
	json configdocument;
	std::ifstream configfile("../config.json");
	configfile >> configdocument;
	dpp::cluster bot(configdocument["token"]);

	/* Create command handler, and specify prefixes */
	dpp::commandhandler command_handler(&bot);
	/* Specifying a prefix of "/" tells the command handler it should also expect slash commands */
	command_handler.add_prefix("/");

	bot.on_ready([&bot, &command_handler](const dpp::ready_t & event) {
		command_handler.add_command("avatar", /*avatar*/
		{ {"user", dpp::param_info(dpp::pt_user, true, "Optional user parameter")} },
			[&command_handler, &bot](const std::string& command, const dpp::parameter_list_t& parameters, dpp::command_source src) {
				std::cout << "Command triggered: " << command << "\n";
				std::cout << "REST ping: " << bot.rest_ping << "\n";
				std::cout << "Parameter: \n";
				if (parameters.size() == 0) {
					std::cout << "Optional parameter not provided\n";
				} else {
					dpp::resolved_user ru = std::get<dpp::resolved_user>(parameters[0].second);
					std::cout << "Member user id: " << ru.member.user_id << "\n";
					std::cout << "Member guild id: " << ru.member.guild_id << "\n";
					std::cout << "User id: " << ru.user.id << "\n";
					std::cout << "User detail: " << ru.user.username << "#" << ru.user.discriminator << "\n";
				}
				command_handler.reply(dpp::message(src.channel_id, "This is a reply with attachment").set_file_content("Ooga Booga").set_filename("ooga_booga.txt").set_flags(dpp::m_ephemeral), src);
		}, "Get the avatar from user", 825407338755653642);

		/* This is a dummy for testing export */
		dpp::user* u = dpp::find_user(1234567890123);

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
