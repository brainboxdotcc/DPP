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

	dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content);

	FILE *fd;
	fd = fopen("./me.pcm", "wb");

        bot.on_log(dpp::utility::cout_logger());

	/* Use the on_message_create event to look for commands */
	bot.on_message_create([&bot, &fd](const dpp::message_create_t & event) {

		std::stringstream ss(event.msg.content);
		std::string command;

		ss >> command;

		/* Tell the bot to record */
		if (command == ".record") {
			dpp::guild * g = dpp::find_guild(event.msg.guild_id);
			if (!g->connect_member_voice(event.msg.author.id)) {
				bot.message_create(dpp::message(
					event.msg.channel_id, 
					"You don't seem to be on a voice channel! :("
				));
			}
		}

		/* Tell the bot to stop recording */
		if (command == ".stop") {
			event.from->disconnect_voice(event.msg.guild_id);
			fclose(fd);
		}
	});

	bot.on_voice_receive([&bot, &fd, &user_id](const dpp::voice_receive_t &event) {
		if (event.user_id == user_id) {
			fwrite((char *)event.audio, 1, event.audio_size, fd);
		}
	});

	/* Start bot */
	bot.start(dpp::st_wait);
	return 0;
}
~~~~~~~~~~

