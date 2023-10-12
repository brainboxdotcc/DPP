#include <dpp/dpp.h>
#include <fmt/format.h>
#include <iomanip>
#include <sstream>

#include <vector>
#include <fstream>
#include <iostream>
#include <mpg123.h>
#include <out123.h>

/* For an example we will hardcode a path to some awesome music here */
#define MUSIC_FILE "/media/music/Rick Astley/Whenever You Need Somebody/Never Gonna Give You Up.mp3"

int main() {
	/* This will hold the decoded MP3.
	* The D++ library expects PCM format, which are raw sound
	* data, 2 channel stereo, 16 bit signed 48000Hz.
	*/
	std::vector<uint8_t> pcmdata;

	mpg123_init();

	int err = 0;
	unsigned char* buffer;
	size_t buffer_size, done;
	int channels, encoding;
	long rate;

	/* Note it is important to force the frequency to 48000 for Discord compatibility */
	mpg123_handle *mh = mpg123_new(NULL, &err);
	mpg123_param(mh, MPG123_FORCE_RATE, 48000, 48000.0);

	/* Decode entire file into a vector. You could do this on the fly, but if you do that
	* you may get timing issues if your CPU is busy at the time and you are streaming to
	* a lot of channels/guilds.
	*/
	buffer_size = mpg123_outblock(mh);
	buffer = new unsigned char[buffer_size];

	/* Note: In a real world bot, this should have some error logging */
	mpg123_open(mh, MUSIC_FILE);
	mpg123_getformat(mh, &rate, &channels, &encoding);

	unsigned int counter = 0;
	for (int totalBytes = 0; mpg123_read(mh, buffer, buffer_size, &done) == MPG123_OK; ) {
		for (size_t i = 0; i < buffer_size; i++) {
			pcmdata.push_back(buffer[i]);
		}
		counter += buffer_size;
		totalBytes += done;
	}
	delete[] buffer;
	mpg123_close(mh);
	mpg123_delete(mh);

	/* Setup the bot */
	dpp::cluster bot("token");

	bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot, &pcmdata](const dpp::slashcommand_t& event) {
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
		} else if (event.command.get_command_name() == "mp3") {
			/* Get the voice channel the bot is in, in this current guild. */
			dpp::voiceconn* v = event.from->get_voice(event.command.guild_id);

			/* If the voice channel was invalid, or there is an issue with it, then tell the user. */
			if (!v || !v->voiceclient || !v->voiceclient->is_ready()) {
				event.reply("There was an issue with getting the voice channel. Make sure I'm in a voice channel!");
				return;
			}

			/* Stream the already decoded MP3 file. This passes the PCM data to the library to be encoded to OPUS */
			v->voiceclient->send_audio_raw((uint16_t*)pcmdata.data(), pcmdata.size());

			event.reply("Played the mp3 file.");
		}
	});

	bot.on_ready([&bot](const dpp::ready_t & event) {
		if (dpp::run_once<struct register_bot_commands>()) {
			/* Create a new command. */
			dpp::slashcommand joincommand("join", "Joins your voice channel.", bot.me.id);
			dpp::slashcommand mp3command("mp3", "Plays an mp3 file.", bot.me.id);

			bot.global_bulk_command_create({ joincommand, mp3command });
		}
	});

	/* Start bot */
	bot.start(dpp::st_wait);

	/* Clean up */
	mpg123_exit();

	return 0;
}
