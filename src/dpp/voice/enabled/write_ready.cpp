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
	std::chrono::nanoseconds latency{0};
	if (send_audio_type != satype_live_audio) {
		auto now = std::chrono::high_resolution_clock::now();

		auto minimum = std::chrono::nanoseconds(last_duration);
		auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(now - last_timestamp);

		bool should_send_now = elapsed >= minimum;
		if (!should_send_now) {
			std::this_thread::sleep_for(minimum - elapsed);
			udp_events.flags = WANT_READ | WANT_WRITE | WANT_ERROR;
			owner->socketengine->update_socket(udp_events);
			return;
		}

		latency = elapsed - minimum;
		last_timestamp = now;
	}

	uint64_t duration = 0;
	bool track_marker_found = false;
	uint64_t bufsize = 0;
	{
		std::lock_guard<std::mutex> lock(this->stream_mutex);
		if (this->paused) {
			if (!this->sent_stop_frames) {
				this->send_stop_frames(true);
				this->sent_stop_frames = true;
			}

			/* Fallthrough if paused */
		} else if (!outbuf.empty()) {
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
			if (!outbuf.empty()) {
				udp_events.flags = WANT_READ | WANT_WRITE | WANT_ERROR;
				owner->socketengine->update_socket(udp_events);
			}
		}
	}
	if (duration) {
		auto latcount = latency.count();
		last_duration = duration > latcount ? duration - latcount : duration;
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
