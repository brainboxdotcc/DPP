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
	dpp::cluster bot("token");

    bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {

		/* Check which command they ran */
		if (event.command.get_command_name() == "join") {

			/* Get the guild */
			dpp::guild* g = dpp::find_guild(event.command.guild_id);

			/* Get the voice channel that the bot is currently in from this server (will return nullptr if we're not in a voice channel!) */
			auto current_vc = event.from->get_voice(event.command.guild_id);

			bool join_vc = true;

			/* Are we in a voice channel? If so, let's see if we're in the right channel. */
			if (current_vc) {
				/* Find the channel id that the user is currently in */
				auto users_vc = g->voice_members.find(event.command.get_issuing_user().id);
				
				if (users_vc != g->voice_members.end() && current_vc->channel_id == users_vc->second.channel_id) {
					join_vc = false;
					
					/* We are on this voice channel, at this point we can send any audio instantly to vc:

					 * current_vc->send_audio_raw(...)
					 */
				} else {
					/* We are on a different voice channel. We should leave it, then join the new one 
					 * by falling through to the join_vc branch below.
					 */
					event.from->disconnect_voice(event.command.guild_id);

					join_vc = true;
				}
			}

			/* If we need to join a vc at all, join it here if join_vc == true */
			if(join_vc) {
				/* Attempt to connect to a voice channel, returns false if we fail to connect. */

				/* The user issuing the command is not on any voice channel, we can't do anything */
				if (!g->connect_member_voice(event.command.get_issuing_user().id)) {
					event.reply("You don't seem to be in a voice channel!");
					return;
				}

				/* We are now connecting to a vc. Wait for on_voice_ready 
				 * event, and then send the audio within that event:
				 * 
				 * event.voice_client->send_audio_raw(...);
				 * 
				 * NOTE: We can't instantly send audio, as we have to wait for
				 * the connection to the voice server to be established!
				 */

				/* Tell the user we joined their channel. */
				event.reply("Joined your channel!");
			} else {
				event.reply("Don't need to join your channel as i'm already there with you!");
			}
		}
	});

	bot.on_ready([&bot](const dpp::ready_t & event) {
		if (dpp::run_once<struct register_bot_commands>()) {

			/* Create a new command. */
			bot.global_command_create(dpp::slashcommand("join", "Joins your voice channel.", bot.me.id));
		}
	});

	/* Start bot */
	bot.start(dpp::st_wait);

	return 0;
}

~~~~~~~~~~~~~~~~~~~~~~~~~