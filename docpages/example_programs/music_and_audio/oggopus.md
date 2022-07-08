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
	dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content);

        bot.on_log(dpp::utility::cout_logger());

	/* Use the on_message_create event to look for commands */
	bot.on_message_create([&bot](const dpp::message_create_t & event) {
		std::stringstream ss(event.msg.content);
		std::string command;
		ss >> command;

		/* Tell the bot to join the discord voice channel the user is on. Syntax: .join */
		if (command == ".join") {
			dpp::guild * g = dpp::find_guild(event.msg.guild_id);
			if (!g->connect_member_voice(event.msg.author.id)) {
				bot.message_create(dpp::message(event.msg.channel_id, "You don't seem to be on a voice channel! :("));
			}
		}

		/* Tell the bot to play the sound file */
		if (command == ".play") {
			dpp::voiceconn* v = event.from->get_voice(event.msg.guild_id);
			if (v && v->voiceclient && v->voiceclient->is_ready()) {
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
				if (ogg_sync_pageout(&oy, &og) != 1)
				{
					fprintf(stderr,"Does not appear to be ogg stream.\n");
					exit(1);
				}

				ogg_stream_init(&os, ogg_page_serialno(&og));

				if (ogg_stream_pagein(&os,&og) < 0) {
					fprintf(stderr,"Error reading initial page of ogg stream.\n");
					exit(1);
				}

				if (ogg_stream_packetout(&os,&op) != 1)
				{
					fprintf(stderr,"Error reading header packet of ogg stream.\n");
					exit(1);
				}

				/* We must ensure that the ogg stream actually contains opus data */
				if (!(op.bytes > 8 && !memcmp("OpusHead", op.packet, 8)))
				{
					fprintf(stderr,"Not an ogg opus stream.\n");
					exit(1);
				}

				/* Parse the header to get stream info */
				int err = opus_head_parse(&header, op.packet, op.bytes);
				if (err)
				{
					fprintf(stderr,"Not a ogg opus stream\n");
					exit(1);
				}
				/* Now we ensure the encoding is correct for Discord */
				if (header.channel_count != 2 && header.input_sample_rate != 48000)
				{
					fprintf(stderr,"Wrong encoding for Discord, must be 48000Hz sample rate with 2 channels.\n");
					exit(1);
				}

				/* Now loop though all the pages and send the packets to the vc */
				while (ogg_sync_pageout(&oy, &og) == 1){
					ogg_stream_init(&os, ogg_page_serialno(&og));

					if(ogg_stream_pagein(&os,&og)<0){
						fprintf(stderr,"Error reading page of Ogg bitstream data.\n");
						exit(1);
					}

					while (ogg_stream_packetout(&os,&op) != 0)
					{
						/* Read remaining headers */
						if (op.bytes > 8 && !memcmp("OpusHead", op.packet, 8))
						{
							int err = opus_head_parse(&header, op.packet, op.bytes);
							if (err)
							{
								fprintf(stderr,"Not a ogg opus stream\n");
								exit(1);
							}
							if (header.channel_count != 2 && header.input_sample_rate != 48000)
							{
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
			}
		}
	});
	
	/* Start bot */
	bot.start(false);
	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~~~

You can compile this example using the following command

	c++ /path/to/source.cc -ldpp -lopus -lopusfile -logg -I/usr/include/opus

