\page oggopus Streaming Ogg Opus file

This example shows how to stream an Ogg Opus file to a voice channel. This example requires some additional dependencies, namely `libogg` and `opusfile`.

~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <iomanip>
#include <sstream>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ogg/ogg.h>
#include <opus/opusfile.h>

int main(int argc, char const *argv[])
{
	/* Load an ogg opus file into memory.
	 * The bot expects opus packets to be 2 channel stereo, 48000Hz.
	 * 
	 * You may use ffmpeg to encode songs to ogg opus:
	 * ffmpeg -i /path/to/song -c:a libopus -ar 48000 -ac 2 -vn -b:a 96K /path/to/opus.ogg 
	 */

	dpp::cluster bot("token");

    bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {

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
		} else if (event.command.get_command_name() == "play") {

			/* Get the voice channel the bot is in, in this current guild. */
			dpp::voiceconn* v = event.from->get_voice(event.channel.guild_id);

			/* If the voice channel was invalid, or there is an issue with it, then tell the user. */
			if (!v || !v->voiceclient || !v->voiceclient->is_ready()) {
				event.reply("There was an issue with getting the voice channel. Make sure I'm in a voice channel!");
				return;
			}

			ogg_sync_state oy; 
			ogg_stream_state os;
			ogg_page og;
			ogg_packet op;
			OpusHead header;
			char *buffer;

			FILE *fd;

			fd = fopen("/path/to/opus.ogg", "rb");

			fseek(fd, 0L, SEEK_END);
			size_t sz = ftell(fd);
			rewind(fd);

			ogg_sync_init(&oy);

			int eos = 0;
			int i;

			buffer = ogg_sync_buffer(&oy, sz);
			fread(buffer, 1, sz, fd);

			ogg_sync_wrote(&oy, sz);

			/**
				* We must first verify that the stream is indeed ogg opus
				* by reading the header and parsing it
				*/
			if (ogg_sync_pageout(&oy, &og) != 1) {
				fprintf(stderr,"Does not appear to be ogg stream.\n");
				exit(1);
			}

			ogg_stream_init(&os, ogg_page_serialno(&og));

			if (ogg_stream_pagein(&os,&og) < 0) {
				fprintf(stderr,"Error reading initial page of ogg stream.\n");
				exit(1);
			}

			if (ogg_stream_packetout(&os,&op) != 1) {
				fprintf(stderr,"Error reading header packet of ogg stream.\n");
				exit(1);
			}

			/* We must ensure that the ogg stream actually contains opus data */
			if (!(op.bytes > 8 && !memcmp("OpusHead", op.packet, 8))) {
				fprintf(stderr,"Not an ogg opus stream.\n");
				exit(1);
			}

			/* Parse the header to get stream info */
			int err = opus_head_parse(&header, op.packet, op.bytes);
			if (err) {
				fprintf(stderr,"Not a ogg opus stream\n");
				exit(1);
			}

			/* Now we ensure the encoding is correct for Discord */
			if (header.channel_count != 2 && header.input_sample_rate != 48000) {
				fprintf(stderr,"Wrong encoding for Discord, must be 48000Hz sample rate with 2 channels.\n");
				exit(1);
			}

			/* Now loop though all the pages and send the packets to the vc */
			while (ogg_sync_pageout(&oy, &og) == 1) {
				ogg_stream_init(&os, ogg_page_serialno(&og));

				if(ogg_stream_pagein(&os,&og)<0) {
					fprintf(stderr,"Error reading page of Ogg bitstream data.\n");
					exit(1);
				}

				while (ogg_stream_packetout(&os,&op) != 0) {

					/* Read remaining headers */
					if (op.bytes > 8 && !memcmp("OpusHead", op.packet, 8)) {
						int err = opus_head_parse(&header, op.packet, op.bytes);
						if (err) {
							fprintf(stderr,"Not a ogg opus stream\n");
							exit(1);
						}

						if (header.channel_count != 2 && header.input_sample_rate != 48000) {
							fprintf(stderr,"Wrong encoding for Discord, must be 48000Hz sample rate with 2 channels.\n");
							exit(1);
						}

						continue;
					}

					/* Skip the opus tags */
					if (op.bytes > 8 && !memcmp("OpusTags", op.packet, 8))
						continue; 

					/* Send the audio */
					int samples = opus_packet_get_samples_per_frame(op.packet, 48000);

					v->voiceclient->send_audio_opus(op.packet, op.bytes, samples / 48);
				}
			}

			/* Cleanup */
			ogg_stream_clear(&os);
			ogg_sync_clear(&oy);

			event.reply("Finished playing the audio file!");
		}
	});

	bot.on_ready([&bot](const dpp::ready_t & event) {
		if (dpp::run_once<struct register_bot_commands>()) {

			/* Create a new command. */
			dpp::slashcommand joincommand("join", "Joins your voice channel.", bot.me.id);

			dpp::slashcommand playcommand("play", "Plays an ogg file.", bot.me.id);

			bot.global_bulk_command_create({joincommand, playcommand});
		}
	});
	
	/* Start bot */
	bot.start(dpp::st_wait);

	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~~~

You can compile this example using the following command

	c++ /path/to/source.cc -ldpp -lopus -lopusfile -logg -I/usr/include/opus

## Using liboggz

You can use `liboggz` to stream an Ogg Opus file to discord voice channel.
`liboggz` provides higher level abstraction and useful APIs. Some features `liboggz` provides include: seeking and timestamp interpretation.
Read more on the [documentation](https://www.xiph.org/oggz/doc/).

~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <iomanip>
#include <sstream>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <oggz/oggz.h>

int main(int argc, char const *argv[])
{
	/* Load an ogg opus file into memory.
	 * The bot expects opus packets to be 2 channel stereo, 48000Hz.
	 * 
	 * You may use ffmpeg to encode songs to ogg opus:
	 * ffmpeg -i /path/to/song -c:a libopus -ar 48000 -ac 2 -vn -b:a 96K /path/to/opus.ogg 
	 */

	/* Setup the bot */
	dpp::cluster bot("token");

    bot.on_log(dpp::utility::cout_logger());

	/* The event is fired when someone issues your commands */
	bot.on_slashcommand([&bot](const dpp::slashcommand_t& event) {

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
		} else if (event.command.get_command_name() == "play") {

			/* Get the voice channel the bot is in, in this current guild. */
			dpp::voiceconn* v = event.from->get_voice(event.channel.guild_id);

			/* If the voice channel was invalid, or there is an issue with it, then tell the user. */
			if (!v || !v->voiceclient || !v->voiceclient->is_ready()) {
				event.reply("There was an issue with getting the voice channel. Make sure I'm in a voice channel!");
				return;
			}

			// load the audio file with oggz
			OGGZ *track_og = oggz_open("/path/to/opus.ogg", OGGZ_READ);

			/* If there was an issue reading the file, tell the user and stop */
			if (!track_og) {
				fprintf(stderr, "Error opening file\n");
				event.reply("There was an issue opening the file!");
				return;
			}

			// set read callback, this callback will be called on packets with the serialno,
			// -1 means every packet will be handled with this callback
			oggz_set_read_callback(
				track_og, -1,
				[](OGGZ *oggz, oggz_packet *packet, long serialno,
					void *user_data) {
					dpp::voiceconn *voiceconn = (dpp::voiceconn *)user_data;

					// send the audio
					voiceconn->voiceclient->send_audio_opus(packet->op.packet,
							packet->op.bytes);

					// make sure to always return 0 here, read more on oggz documentation
					return 0;
				},
				// this will be the value of void *user_data
				(void *)v
			);

			// read loop
			while (v && v->voiceclient && !v->voiceclient->terminating) {
				// you can tweak this to whatever. Here I use BUFSIZ, defined in
				// stdio.h as 8192
				static const constexpr long CHUNK_READ = BUFSIZ * 2;

				const long read_bytes = oggz_read(track_og, CHUNK_READ);

				// break on eof
				if (!read_bytes)
					break;
			}

			// don't forget to free the memory
			oggz_close(track_og);

			event.reply("Finished playing the audio file!");
		}
	});

	bot.on_ready([&bot](const dpp::ready_t & event) {
		if (dpp::run_once<struct register_bot_commands>()) {

			/* Create a new command. */
			dpp::slashcommand joincommand("join", "Joins your voice channel.", bot.me.id);

			dpp::slashcommand playcommand("play", "Plays an ogg file.", bot.me.id);

			bot.global_bulk_command_create({joincommand, playcommand});
		}
	});
	
	/* Start bot */
	bot.start(dpp::st_wait);
	
	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~~~

You can compile this example using the following command:

	c++ /path/to/source.cc -ldpp -loggz