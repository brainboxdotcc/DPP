\page soundboard Creating a Sound Board

This example script shows how to send a sound file to a voice channel. A few shortcuts are taken here, for more advanced techniques for connecting to a voice channel see the tutorial \ref joinvc

~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <iomanip>
#include <sstream>

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
	std::ifstream input ("../testdata/Robot.pcm", std::ios::in|std::ios::binary|std::ios::ate);
	if (input.is_open()) {
		robot_size = input.tellg();
		robot = new uint8_t[robot_size];
		input.seekg (0, std::ios::beg);
		input.read ((char*)robot, robot_size);
		input.close();
	}

	/* Setup the bot */
	dpp::cluster bot("token");

    bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot, robot, robot_size](const dpp::slashcommand_t& event) {

		/* Check which command they ran */
		if (event.command.get_command_name() == "join") {

			/* Get the guild */
			dpp::guild* g = dpp::find_guild(event.command.guild_id);

			/* Attempt to connect to a voice channel, returns false if we fail to connect. */
			if (!g->connect_member_voice(event.command.get_issuing_user().id)) {
				event.reply("You don't seem to be in a voice channel!");
				return;
			}
			
			/* Tell the user we joined their channel. */
			event.reply("Joined your channel!");
		} else if (event.command.get_command_name() == "robot") {

			/* Get the voice channel the bot is in, in this current guild. */
			dpp::voiceconn* v = event.from->get_voice(event.channel.guild_id);

			/* If the voice channel was invalid, or there is an issue with it, then tell the user. */
			if (!v || !v->voiceclient || !v->voiceclient->is_ready()) {
				event.reply("There was an issue with getting the voice channel. Make sure I'm in a voice channel!");
				return;
			}

			/* Tell the bot to play the sound file 'Robot.pcm' in the current voice channel. */
			v->voiceclient->send_audio_raw((uint16_t*)robot, robot_size);

			event.reply("Played robot.");
		}
	});

	bot.on_ready([&bot](const dpp::ready_t & event) {
		if (dpp::run_once<struct register_bot_commands>()) {

			/* Create a new command. */
			dpp::slashcommand joincommand("join", "Joins your voice channel.", bot.me.id);

			dpp::slashcommand robotcommand("robot", "Plays a robot noise in your voice channel.", bot.me.id);

			bot.global_bulk_command_create({joincommand, robotcommand});
		}
	});

	/* Start bot */
	bot.start(dpp::st_wait);

	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~