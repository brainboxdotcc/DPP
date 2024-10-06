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

#include <string_view>
#include <dpp/exception.h>
#include <dpp/isa_detection.h>
#include <dpp/discordvoiceclient.h>

#include <opus/opus.h>
#include "../../dave/encryptor.h"

#include "enabled.h"

namespace dpp {

void discord_voice_client::voice_courier_loop(discord_voice_client& client, courier_shared_state_t& shared_state) {
	utility::set_thread_name(std::string("vcourier/") + std::to_string(client.server_id));
	while (true) {
		std::this_thread::sleep_for(std::chrono::milliseconds{client.iteration_interval});

		struct flush_data_t {
			snowflake user_id;
			rtp_seq_t min_seq;
			std::priority_queue<voice_payload> parked_payloads;
			std::vector<std::function<void(OpusDecoder &)>> pending_decoder_ctls;
			std::shared_ptr<OpusDecoder> decoder;
		};
		std::vector<flush_data_t> flush_data;

		/*
		 * Transport the payloads onto this thread, and
		 * release the lock as soon as possible.
		 */
		{
			std::unique_lock lk(shared_state.mtx);

			/* mitigates vector resizing while holding the mutex */
			flush_data.reserve(shared_state.parked_voice_payloads.size());

			bool has_payload_to_deliver = false;
			for (auto &[user_id, parking_lot]: shared_state.parked_voice_payloads) {
				has_payload_to_deliver = has_payload_to_deliver || !parking_lot.parked_payloads.empty();
				flush_data.push_back(flush_data_t{user_id,
								  parking_lot.range.min_seq,
								  std::move(parking_lot.parked_payloads),
					/* Quickly check if we already have a decoder and only take the pending ctls if so. */
								  parking_lot.decoder ? std::move(parking_lot.pending_decoder_ctls)
										      : decltype(parking_lot.pending_decoder_ctls){},
								  parking_lot.decoder});
				parking_lot.range.min_seq = parking_lot.range.max_seq + 1;
				parking_lot.range.min_timestamp = parking_lot.range.max_timestamp + 1;
			}

			if (!has_payload_to_deliver) {
				if (shared_state.terminating) {
					/* We have delivered all data to handlers. Terminate now. */
					break;
				}

				shared_state.signal_iteration.wait(lk);
				/*
				 * More data came or about to terminate, or just a spurious wake.
				 * We need to collect the payloads again to determine what to do next.
				 */
				continue;
			}
		}

		if (client.creator->on_voice_receive.empty() && client.creator->on_voice_receive_combined.empty()) {
			/*
			 * We do this check late, to ensure this thread drains the data
			 * and prevents accumulating them even when there are no handlers.
			 */
			continue;
		}

		/* This 32 bit PCM audio buffer is an upmixed version of the streams
		 * combined for all users. This is a wider width audio buffer so that
		  * there is no clipping when there are many loud audio sources at once.
		 */
		opus_int32 pcm_mix[23040] = {0};
		size_t park_count = 0;
		int max_samples = 0;
		int samples = 0;

		for (auto &d: flush_data) {
			if (!d.decoder) {
				continue;
			}
			for (const auto &decoder_ctl: d.pending_decoder_ctls) {
				decoder_ctl(*d.decoder);
			}
			for (rtp_seq_t seq = d.min_seq; !d.parked_payloads.empty(); ++seq) {
				opus_int16 pcm[23040];
				if (d.parked_payloads.top().seq != seq) {
					/*
					 * Lost a packet with sequence number "seq",
					 * But Opus decoder might be able to guess something.
					 */
					if (int samples = opus_decode(d.decoder.get(), nullptr, 0, pcm, 5760, 0);
						samples >= 0) {
						/*
						 * Since this sample comes from a lost packet,
						 * we can only pretend there is an event, without any raw payload byte.
						 */
						voice_receive_t vr(nullptr, "", &client, d.user_id, reinterpret_cast<uint8_t *>(pcm),
								   samples * opus_channel_count * sizeof(opus_int16));

						park_count = audio_mix(client, *client.mixer, pcm_mix, pcm, park_count, samples, max_samples);
						client.creator->on_voice_receive.call(vr);
					}
				} else {
					voice_receive_t &vr = *d.parked_payloads.top().vr;
					if (vr.audio_data.size() > 0x7FFFFFFF) {
						throw dpp::length_exception(err_massive_audio, "audio_data > 2GB! This should never happen!");
					}
					if (samples = opus_decode(d.decoder.get(), vr.audio_data.data(),
								  static_cast<opus_int32>(vr.audio_data.size() & 0x7FFFFFFF), pcm, 5760, 0);
						samples >= 0) {
						vr.reassign(&client, d.user_id, reinterpret_cast<uint8_t *>(pcm),
							    samples * opus_channel_count * sizeof(opus_int16));
						client.end_gain = 1.0f / client.moving_average;
						park_count = audio_mix(client, *client.mixer, pcm_mix, pcm, park_count, samples, max_samples);
						client.creator->on_voice_receive.call(vr);
					}

					d.parked_payloads.pop();
				}
			}
		}

		/* If combined receive is bound, dispatch it */
		if (park_count) {

			/* Downsample the 32 bit samples back to 16 bit */
			opus_int16 pcm_downsample[23040] = {0};
			opus_int16 *pcm_downsample_ptr = pcm_downsample;
			opus_int32 *pcm_mix_ptr = pcm_mix;
			client.increment = (client.end_gain - client.current_gain) / static_cast<float>(samples);
			for (int64_t x = 0; x < (samples * opus_channel_count) / client.mixer->byte_blocks_per_register; ++x) {
				client.mixer->collect_single_register(pcm_mix_ptr, pcm_downsample_ptr, client.current_gain, client.increment);
				client.current_gain += client.increment * static_cast<float>(client.mixer->byte_blocks_per_register);
				pcm_mix_ptr += client.mixer->byte_blocks_per_register;
				pcm_downsample_ptr += client.mixer->byte_blocks_per_register;
			}

			voice_receive_t vr(nullptr, "", &client, 0, reinterpret_cast<uint8_t *>(pcm_downsample),
					   max_samples * opus_channel_count * sizeof(opus_int16));

			client.creator->on_voice_receive_combined.call(vr);
		}
	}
}

}
