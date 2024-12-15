/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * SPDX-License-Identifier: Apache-2.0
 * Copyright 2021 Craig Edwards and D++ contributors
 * (https://github.com/brainboxdotcc/DPP/graphs/contributors)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ************************************************************************************/

#include <dpp/exception.h>
#include <dpp/isa_detection.h>
#include <dpp/discordvoiceclient.h>

#include "../../dave/encryptor.h"

#include "enabled.h"

namespace dpp {

void discord_voice_client::write_ready() {
	/* 
	 * WANT_WRITE has been reset everytime this method is being called,
	 * ALWAYS set it again no matter what we're gonna do.
	 */
	udp_events.flags = WANT_READ | WANT_WRITE | WANT_ERROR;
	owner->socketengine->update_socket(udp_events);

	uint64_t duration = 0;
	bool track_marker_found = false;
	uint64_t bufsize = 0;
	send_audio_type_t type = satype_recorded_audio;
	{
		std::lock_guard<std::mutex> lock(this->stream_mutex);
		if (this->paused) {
			if (!this->sent_stop_frames) {
				this->send_stop_frames(true);
				this->sent_stop_frames = true;
			}

			/* Fallthrough if paused */
		} else if (!outbuf.empty()) {
			type = send_audio_type;
			if (outbuf[0].packet.size() == sizeof(uint16_t) && (*(reinterpret_cast<uint16_t*>(outbuf[0].packet.data()))) == AUDIO_TRACK_MARKER) {
				outbuf.erase(outbuf.begin());
				track_marker_found = true;
				if (tracks > 0) {
					tracks--;
				}
			}
			if (!outbuf.empty()) {
				int sent_siz = this->udp_send(outbuf[0].packet.data(), outbuf[0].packet.length());
				if (sent_siz == (int)outbuf[0].packet.length()) {
					duration = outbuf[0].duration * timescale;
					bufsize = outbuf[0].packet.length();
					outbuf.erase(outbuf.begin());
				}
			}
		}
	}
	if (duration) {
		if (type == satype_recorded_audio) {
			std::chrono::nanoseconds latency = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - last_timestamp);
			std::chrono::nanoseconds sleep_time = std::chrono::nanoseconds(duration) - latency;
			if (sleep_time.count() > 0) {
				std::this_thread::sleep_for(sleep_time);
			}
		}
		else if (type == satype_overlap_audio) {
			std::chrono::nanoseconds latency = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - last_timestamp);
			std::chrono::nanoseconds sleep_time = std::chrono::nanoseconds(duration) + last_sleep_remainder - latency;
			std::chrono::nanoseconds sleep_increment = (std::chrono::nanoseconds(duration) - latency) / AUDIO_OVERLAP_SLEEP_SAMPLES;
			if (sleep_time.count() > 0) {
				uint16_t samples_count = 0;
				std::chrono::nanoseconds overshoot_accumulator{};

				do {
					std::chrono::high_resolution_clock::time_point start_sleep = std::chrono::high_resolution_clock::now();
					std::this_thread::sleep_for(sleep_increment);
					std::chrono::high_resolution_clock::time_point end_sleep = std::chrono::high_resolution_clock::now();

					samples_count++;
					overshoot_accumulator += std::chrono::duration_cast<std::chrono::nanoseconds>(end_sleep - start_sleep) - sleep_increment;
					sleep_time -= std::chrono::duration_cast<std::chrono::nanoseconds>(end_sleep - start_sleep);
				} while (std::chrono::nanoseconds(overshoot_accumulator.count() / samples_count) + sleep_increment < sleep_time);
				last_sleep_remainder = sleep_time;
			} else {
				last_sleep_remainder = std::chrono::nanoseconds(0);
			}
		}

		last_timestamp = std::chrono::high_resolution_clock::now();
		if (!creator->on_voice_buffer_send.empty()) {
			voice_buffer_send_t snd(owner, 0, "");
			snd.buffer_size = bufsize;
			snd.packets_left = outbuf.size();
			snd.voice_client = this;
			creator->queue_work(-1, [this, snd]() {
				creator->on_voice_buffer_send.call(snd);
			});
		}
	}
	if (track_marker_found) {
		if (!creator->on_voice_track_marker.empty()) {
			voice_track_marker_t vtm(owner, 0, "");
			vtm.voice_client = this;
			{
				std::lock_guard<std::mutex> lock(this->stream_mutex);
				if (!track_meta.empty()) {
					vtm.track_meta = track_meta[0];
					track_meta.erase(track_meta.begin());
				}
			}
			creator->queue_work(-1, [this, vtm]() {
				creator->on_voice_track_marker.call(vtm);
			});

		}
	}
}

}
