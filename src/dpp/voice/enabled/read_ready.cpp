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

#include <chrono>
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
	                 std::make_unique<voice_receive_t>(owner, 0, std::string(reinterpret_cast<char*>(buffer), packet_size))};

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

	vp.vr->audio_data.assign(buffer, buffer + packet_size);

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
		voice_courier = std::thread(&voice_courier_loop, std::ref(*this), std::ref(voice_courier_shared_state));
	}
}

}
