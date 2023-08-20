\page record-user Record yourself in a VC

DPP supports receiving audio. This examples show how to use it to record some user in a VC.

\note Voice receiving by bots is not officially supported by the Discord API. We cannot guarantee that this feature will work in the future. 

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <iomanip>
#include <sstream>

int main(int argc, char const *argv[])
{
	/* Example to record a user in a VC
	* 
	* Recording is output as './me.pcm' and you can play it via the soundboard example
	* or use ffmpeg 'ffplay -f s16le -ar 48000 -ac 2 -i ./me.pcm'
	*/

	/* Replace with the user's id you wish to record */
	dpp::snowflake user_id = 407877550216314882;

	/* Setup the bot */
	dpp::cluster bot("token");

	FILE *fd;
	fd = fopen("./me.pcm", "wb");

    bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot, &fd](const dpp::slashcommand_t& event) {

		/* Check which command they ran */
		if (event.command.get_command_name() == "record") {

			/* Get the guild */
			dpp::guild* g = dpp::find_guild(event.command.guild_id);

			/* Attempt to connect to a voice channel, returns false if we fail to connect. */
			if (!g->connect_member_voice(event.command.get_issuing_user().id)) {
				event.reply("You don't seem to be in a voice channel!");
				return;
			}
			
			/* Tell the user we joined their channel. */
			event.reply("Joined your channel, now recording!");
		} else if (event.command.get_command_name() == "stop") {

			event.from->disconnect_voice(event.command.guild_id);
			fclose(fd);

			event.reply("Stopped recording.");
		}
	});

	bot.on_voice_receive([&bot, &fd, &user_id](const dpp::voice_receive_t &event) {
		if (event.user_id == user_id) {
			fwrite((char *)event.audio, 1, event.audio_size, fd);
		}
	});

	bot.on_ready([&bot](const dpp::ready_t & event) {
		if (dpp::run_once<struct register_bot_commands>()) {

			/* Create a new command. */
			dpp::slashcommand recordcommand("record", "Joins your voice channel and records you.", bot.me.id);

			dpp::slashcommand stopcommand("stop", "Stops recording you.", bot.me.id);

			bot.global_bulk_command_create({recordcommand, stopcommand});
		}
	});

	/* Start bot */
	bot.start(dpp::st_wait);

	return 0;
}
~~~~~~~~~~

