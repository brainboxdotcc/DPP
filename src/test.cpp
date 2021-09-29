#undef DPP_BUILD

#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>
#include <dpp/fmt/format.h>
#include <iomanip>
#include <sstream>

using json = nlohmann::json;

int main(int argc, char const *argv[])
{
	/* Load a sound file called Robot.pcm into memory.
	* The bot expects PCM format, which are raw sound data,
	* 2 channel stereo, 16 bit signed 48000Hz.
	* 
	* You can use audacity to export these from WAV or MP3 etc.
	* 
	* If you wanted to send a more complicated format, you could
	* use a separate library to decode that audio to PCM. For
	* example purposes, a raw PCM will suffice. This PCM file can
	* be found within the bot's github repo.
	*/
	uint8_t* robot = nullptr;
	size_t robot_size = 0;
	std::ifstream input ("../testdata/onandon.pcm", std::ios::in|std::ios::binary|std::ios::ate);
	if (input.is_open()) {
		robot_size = input.tellg();
		robot = new uint8_t[robot_size];
		input.seekg (0, std::ios::beg);
		input.read ((char*)robot, robot_size);
		input.close();
	}

	/* Setup the bot */
	json configdocument;
	std::ifstream configfile("../config.json");
	configfile >> configdocument;
	dpp::cluster bot(configdocument["token"]);
	/* Use the on_message_create event to look for commands */
	bot.on_message_create([&bot, robot, robot_size](const dpp::message_create_t & event) {
		std::stringstream ss(event.msg->content);
		std::string command;
		ss >> command;

		/* Tell the bot to join the discord voice channel the user is on. Syntax: .join */
		if (command == ".join") {
			dpp::guild * g = dpp::find_guild(event.msg->guild_id);
			if (!g->connect_member_voice(event.msg->author->id)) {
				bot.message_create(dpp::message(event.msg->channel_id, "You don't seem to be on a voice channel! :("));
			}
		}

		/* Tell the bot to play the sound file 'Robot.pcm'. Syntax: .robot */
		if (command == ".maxpower") {
			dpp::voiceconn* v = event.from->get_voice(event.msg->guild_id);
			if (v && v->voiceclient && v->voiceclient->is_ready()) {
				v->voiceclient->send_audio_raw((uint16_t*)robot, robot_size);
			}
		}
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

	/* Start bot */
	bot.start(false);

	return 0;
}