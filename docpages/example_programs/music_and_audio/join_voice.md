\page joinvc Join or switch to the voice channel of the user issuing a command

When a user issues a command you may want to join their voice channel, e.g. in a music bot. If you are already on the same voice channel, the bot should do nothing (but be ready to instantly play audio) and if the user is on a different voice channel, the bot should switch to it. If the user is on no voice channel at all, this should be considered an error. This example shows how to do this.

\note Please be aware this example sends no audio, but indicates clearly in the comments where and how you should do so.

~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <iomanip>
#include <sstream>

int main(int argc, char const *argv[])
{
	/* Setup the bot */
	dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content); // Privileged intent required to receive message content

        bot.on_log(dpp::utility::cout_logger());

	/* Use the on_message_create event to look for commands */
	bot.on_message_create([&bot, robot, robot_size](const dpp::message_create_t & event) {

		std::stringstream ss(event.msg.content);
		std::string command;

		ss >> command;

		/* Switch to or join the vc the user is on. Syntax: .join  */
		if (command == ".join") {
			dpp::guild * g = dpp::find_guild(event.msg.guild_id);
			auto current_vc = event.from->get_voice(event.msg.guild_id);
			bool join_vc = true;
			/* Check if we are currently on any vc */
			if (current_vc) {
				/* Find the channel id  that the user is currently on */
				auto users_vc = g->voice_members.find(event.msg.author.id);
				/* See if we currently share a channel with the user */
				if (users_vc != g->voice_members.end() && current_vc->channel_id == users_vc->second.channel_id) {
					join_vc = false;
					/* We are on this voice channel, at this point we can send any audio instantly to vc:

					 * current_vc->send_audio_raw(...)
					 */
				} else {
					/* We are on a different voice channel. Leave it, then join the new one 
					 * by falling through to the join_vc branch below.
					 */
					event.from->disconnect_voice(event.msg.guild_id);
					join_vc = true;
				}
			}
			/* If we need to join a vc at all, join it here if join_vc == true */
			if (join_vc) {
				if (!g->connect_member_voice(event.msg.author.id)) {
					/* The user issuing the command is not on any voice channel, we can't do anything */
					bot.message_create(dpp::message(event.msg.channel_id, "You don't seem to be on a voice channel! :("));
				} else {
					/* We are now connecting to a vc. Wait for on_voice_ready 
					 * event, and then send the audio within that event:
					 * 
					 * event.voice_client->send_audio_raw(...);
					 * 
					 * NOTE: We can't instantly send audio, as we have to wait for
					 * the connection to the voice server to be established!
					 */
				}
			}
		}
	});

	/* Start bot */
	bot.start(dpp::st_wait);
	return 0;
}

~~~~~~~~~~~~~~~~~~~~~~~~~
