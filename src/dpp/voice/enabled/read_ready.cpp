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
#include "../../dave/decryptor.h"

#include "enabled.h"

namespace dpp {

void discord_voice_client::read_ready()
{
	uint8_t buffer[65535];
	int packet_size = this->udp_recv(reinterpret_cast<char*>(buffer), sizeof(buffer));

	bool receive_handler_is_empty = creator->on_voice_receive.empty() && creator->on_voice_receive_combined.empty();
	if (packet_size <= 0 || receive_handler_is_empty) {
		/* Nothing to do */
		return;
	}

	constexpr size_t header_size = 12;
	if (static_cast<size_t>(packet_size) < header_size) {
		/* Invalid RTP payload */
		return;
	}

	/* It's a "silence packet" - throw it away. */
	if (packet_size < 44) {
		return;
	}

	if (uint8_t payload_type = buffer[1] & 0b0111'1111;
		72 <= payload_type && payload_type <= 76) {
		/*
		 * This is an RTCP payload. Discord is known to send
		 * RTCP Receiver Reports.
		 *
		 * See https://datatracker.ietf.org/doc/html/rfc3551#section-6
		 */
		return;
	}

	voice_payload vp{0, // seq, populate later
			 0, // timestamp, populate later
			 std::make_unique<voice_receive_t>(nullptr, std::string(reinterpret_cast<char*>(buffer), packet_size))};

	vp.vr->voice_client = this;

	uint32_t speaker_ssrc;
	{	/* Get the User ID of the speaker */
		std::memcpy(&speaker_ssrc, &buffer[8], sizeof(uint32_t));
		speaker_ssrc = ntohl(speaker_ssrc);
		vp.vr->user_id = ssrc_map[speaker_ssrc];
	}

	/* Get the sequence number of the voice UDP packet */
	std::memcpy(&vp.seq, &buffer[2], sizeof(rtp_seq_t));
	vp.seq = ntohs(vp.seq);

	/* Get the timestamp of the voice UDP packet */
	std::memcpy(&vp.timestamp, &buffer[4], sizeof(rtp_timestamp_t));
	vp.timestamp = ntohl(vp.timestamp);

	constexpr size_t nonce_size = sizeof(uint32_t);
	/* Nonce is 4 byte at the end of payload with zero padding */
	uint8_t nonce[24] = { 0 };
	std::memcpy(nonce, buffer + packet_size - nonce_size, nonce_size);

	/* Get the number of CSRC in header */
	const size_t csrc_count = buffer[0] & 0b0000'1111;
	/* Skip to the encrypted voice data */
	const ptrdiff_t offset_to_data = header_size + sizeof(uint32_t) * csrc_count;
	size_t total_header_len = offset_to_data;

	uint8_t* ciphertext = buffer + offset_to_data;
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

	uint8_t decrypted[65535] = { 0 };
	unsigned long long opus_packet_len  = 0;
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
		nonce, secret_key.data()) != 0) {
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
	std::vector<uint8_t> frame;
	if (is_end_to_end_encrypted()) {
		auto decryptor = mls_state->decryptors.find(vp.vr->user_id);
		if (decryptor != mls_state->decryptors.end()) {
			frame.resize(decryptor->second->get_max_plaintext_byte_size(dave::media_type::media_audio, opus_packet_len));
			size_t enc_len = decryptor->second->decrypt(
				dave::media_type::media_audio,
				dave::make_array_view<const uint8_t>(opus_packet, opus_packet_len),
				dave::make_array_view(frame)
			);
			if (enc_len > 0) {
				opus_packet = frame.data();
				opus_packet_len = enc_len;
			}
		}
	}

	/*
	 * We're left with the decrypted, opus-encoded data.
	 * Park the payload and decode on the voice courier thread.
	 */
	vp.vr->audio_data.assign(opus_packet, opus_packet + opus_packet_len);

	{
		std::lock_guard lk(voice_courier_shared_state.mtx);
		auto& [range, payload_queue, pending_decoder_ctls, decoder] = voice_courier_shared_state.parked_voice_payloads[vp.vr->user_id];

		if (!decoder) {
			/*
			 * Most likely this is the first time we encounter this speaker.
			 * Do some initialization for not only the decoder but also the range.
			 */
			range.min_seq = vp.seq;
			range.min_timestamp = vp.timestamp;

			int opus_error = 0;
			decoder.reset(opus_decoder_create(opus_sample_rate_hz, opus_channel_count, &opus_error),
				      &opus_decoder_destroy);
			if (opus_error) {
				/**
				 * NOTE: The -10 here makes the opus_error match up with values of exception_error_code,
				 * which would otherwise conflict as every C library loves to use values from -1 downwards.
				 */
				throw dpp::voice_exception((exception_error_code)(opus_error - 10), "discord_voice_client::discord_voice_client; opus_decoder_create() failed");
			}
		}

		if (vp.seq < range.min_seq && vp.timestamp < range.min_timestamp) {
			/* This packet arrived too late. We can only discard it. */
			return;
		}
		range.max_seq = vp.seq;
		range.max_timestamp = vp.timestamp;
		payload_queue.push(std::move(vp));
	}

	voice_courier_shared_state.signal_iteration.notify_one();

	if (!voice_courier.joinable()) {
		/* Courier thread is not running, start it */
		voice_courier = std::thread(&voice_courier_loop,
					    std::ref(*this),
					    std::ref(voice_courier_shared_state));
	}
}

}
