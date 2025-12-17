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
#include <utility>
#include <dpp/exception.h>
#include <dpp/isa_detection.h>
#include <dpp/discordvoiceclient.h>

#include <opus/opus.h>
#include "../../dave/encryptor.h"

#include "enabled.h"

namespace dpp {

void discord_voice_client::voice_courier_loop(discord_voice_client& client, courier_shared_state_t& shared_state) {
	utility::set_thread_name(std::string("vcourier/") + std::to_string(client.server_id));

	try {

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

					flush_data.push_back(flush_data_t{
						user_id,
						parking_lot.range.min_seq,
						std::move(parking_lot.parked_payloads),
						/* Quickly check if we already have a decoder and only take the pending ctls if so. */
						parking_lot.decoder ? std::move(parking_lot.pending_decoder_ctls)
								    : decltype(parking_lot.pending_decoder_ctls){},
						parking_lot.decoder
					});

					parking_lot.range.min_seq = parking_lot.range.max_seq + 1;
					parking_lot.range.min_timestamp = parking_lot.range.max_timestamp + 1;
				}

				if (!has_payload_to_deliver) {
					if (shared_state.terminating) {
						/* We have delivered all data to handlers. Terminate now. */
						break;
					}

					shared_state.signal_iteration.wait(lk, [&shared_state]() {
						if (shared_state.terminating) {
							return true;
						}

						/*
						 * Actually check the state we're looking for instead of waking up
						 * everytime read_ready was called.
						 */
						for (auto &[user_id, parking_lot]: shared_state.parked_voice_payloads) {
							if (parking_lot.parked_payloads.empty()) {
								continue;
							}
							return true;
						}
						return false;
					});

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

			opus_int16 flush_data_pcm[23040];
			for (auto &d: flush_data) {
				if (!d.decoder) {
					continue;
				}
				for (const auto &decoder_ctl: d.pending_decoder_ctls) {
					decoder_ctl(*d.decoder);
				}

				for (rtp_seq_t seq = d.min_seq; !d.parked_payloads.empty(); ++seq) {
					if (d.parked_payloads.top().seq != seq) {
						/*
						 * Lost a packet with sequence number "seq",
						 * But Opus decoder might be able to guess something.
						 */
						if (int lost_packet_samples = opus_decode(d.decoder.get(), nullptr, 0, flush_data_pcm, 5760, 0);
							lost_packet_samples >= 0) {
							/*
							 * Since this sample comes from a lost packet,
							 * we can only pretend there is an event, without any raw payload byte.
							 */
							voice_receive_t vr(client.creator, 0, "", &client, d.user_id,
									   reinterpret_cast<uint8_t *>(flush_data_pcm),
									   lost_packet_samples * opus_channel_count * sizeof(opus_int16));

							park_count = audio_mix(client, *client.mixer, pcm_mix, flush_data_pcm, park_count, lost_packet_samples, max_samples);
							client.creator->on_voice_receive.call(vr);
						}
					} else {
						voice_receive_t &vr = *d.parked_payloads.top().vr;

						/*
						 * We do decryption here to avoid blocking ssl_connection and saving cpu time by doing it when needed only.
						 *
						 * NOTE: You do not want to send audio while also listening for on_voice_receive/on_voice_receive_combined.
						 * It will cause gaps in your recording, I have no idea why exactly.
						 */

						constexpr size_t header_size = 12;

						uint8_t *buffer = vr.audio_data.data();
						size_t packet_size = vr.audio_data.size();

						constexpr size_t nonce_size = sizeof(uint32_t);
						/* Nonce is 4 byte at the end of payload with zero padding */
						uint8_t nonce[24] = {0};
						std::memcpy(nonce, buffer + packet_size - nonce_size, nonce_size);

						/* Get the number of CSRC in header */
						const size_t csrc_count = buffer[0] & 0b0000'1111;
						/* Skip to the encrypted voice data */
						const ptrdiff_t offset_to_data = header_size + sizeof(uint32_t) * csrc_count;
						size_t total_header_len = offset_to_data;

						uint8_t *ciphertext = buffer + offset_to_data;
						size_t ciphertext_len = packet_size - offset_to_data - nonce_size;

						size_t ext_len = 0;
						if ([[maybe_unused]] const bool uses_extension = (buffer[0] >> 4) & 0b0001) {
							/**
							 * Get the RTP Extensions size, we only get the size here because
							 * the extension itself is encrypted along with the opus packet
							 */
							{
								uint16_t ext_len_in_words;
								memcpy(&ext_len_in_words, &ciphertext[2], sizeof(uint16_t));
								ext_len_in_words = ntohs(ext_len_in_words);
								ext_len = sizeof(uint32_t) * ext_len_in_words;
							}
							constexpr size_t ext_header_len = sizeof(uint16_t) * 2;
							ciphertext += ext_header_len;
							ciphertext_len -= ext_header_len;
							total_header_len += ext_header_len;
						}

						uint8_t decrypted[65535] = {0};
						unsigned long long opus_packet_len = 0;
						if (ssl_crypto_aead_xchacha20poly1305_ietf_decrypt(
							decrypted, &opus_packet_len,
							nullptr,
							ciphertext, ciphertext_len,
							buffer,
							/**
							 * Additional Data:
							 * The whole header (including csrc list) +
							 * 4 byte extension header (magic 0xBEDE + 16-bit denoting extension length)
							 */
							total_header_len,
							nonce, vr.voice_client->secret_key.data()) != 0) {
							/* Invalid Discord RTP payload. */
							return;
						}

						uint8_t *opus_packet = decrypted;
						if (ext_len > 0) {
							/* Skip previously encrypted RTP Header Extension */
							opus_packet += ext_len;
							opus_packet_len -= ext_len;
						}

						/**
						 * If DAVE is enabled, use the user's ratchet to decrypt the OPUS audio data
						 */
						std::vector<uint8_t> decrypted_dave_frame;
						if (vr.voice_client->is_end_to_end_encrypted()) {
							auto decryptor = vr.voice_client->mls_state->decryptors.find(vr.user_id);

							if (decryptor != vr.voice_client->mls_state->decryptors.end()) {
								decrypted_dave_frame.resize(decryptor->second->get_max_plaintext_byte_size(dave::media_type::media_audio, opus_packet_len));

								size_t enc_len = decryptor->second->decrypt(
									dave::media_type::media_audio,
									dave::make_array_view<const uint8_t>(opus_packet, opus_packet_len),
									dave::make_array_view(decrypted_dave_frame)
								);

								if (enc_len > 0) {
									opus_packet = decrypted_dave_frame.data();
									opus_packet_len = enc_len;
								}
							}
						}

						if (opus_packet_len > 0x7FFFFFFF) {
							throw dpp::length_exception(err_massive_audio, "audio_data > 2GB! This should never happen!");
						}

						samples = opus_decode(d.decoder.get(), opus_packet, static_cast<opus_int32>(opus_packet_len & 0x7FFFFFFF), flush_data_pcm, 5760, 0);

						if (samples >= 0) {
							vr.reassign(&client, d.user_id, reinterpret_cast<uint8_t *>(flush_data_pcm), samples * opus_channel_count * sizeof(opus_int16));

							client.end_gain = 1.0f / client.moving_average;
							park_count = audio_mix(client, *client.mixer, pcm_mix, flush_data_pcm, park_count, samples, max_samples);

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

				voice_receive_t vr(client.owner, 0, "", &client, 0, reinterpret_cast<uint8_t *>(pcm_downsample),
						   max_samples * opus_channel_count * sizeof(opus_int16));

				client.creator->on_voice_receive_combined.call(vr);
			}
		}
	}
	catch (const std::exception& e) {
		client.creator->log(ll_critical, "Voice courier unhandled exception: " + std::string(e.what()));
	}
}

}
