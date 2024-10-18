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
#include "../../dave/array_view.h"
#include "../../dave/encryptor.h"

#include "enabled.h"

namespace dpp {

discord_voice_client& discord_voice_client::send_audio_raw(uint16_t* audio_data, const size_t length)  {
	if (length < 4) {
		throw dpp::voice_exception(err_invalid_voice_packet_length, "Raw audio packet size can't be less than 4");
	}

	if ((length % 4) != 0) {
		throw dpp::voice_exception(err_invalid_voice_packet_length, "Raw audio packet size should be divisible by 4");
	}

	if (length > send_audio_raw_max_length) {
		std::string s_audio_data(reinterpret_cast<const char*>(audio_data), length);

		while (s_audio_data.length() > send_audio_raw_max_length) {
			std::string packet(s_audio_data.substr(0, send_audio_raw_max_length));
			const auto packet_size = static_cast<ptrdiff_t>(packet.size());

			s_audio_data.erase(s_audio_data.begin(), s_audio_data.begin() + packet_size);

			send_audio_raw(reinterpret_cast<uint16_t*>(packet.data()), packet_size);
		}

		return *this;
	}

	if (length < send_audio_raw_max_length) {
		std::string packet(reinterpret_cast<const char*>(audio_data), length);
		packet.resize(send_audio_raw_max_length, 0);

		return send_audio_raw(reinterpret_cast<uint16_t*>(packet.data()), packet.size());
	}

	opus_int32 encoded_audio_max_length = (opus_int32)length;
	std::vector<uint8_t> encoded_audio(encoded_audio_max_length);
	size_t encoded_audio_length = encoded_audio_max_length;
	encoded_audio_length = this->encode(reinterpret_cast<uint8_t*>(audio_data), length, encoded_audio.data(), encoded_audio_length);
	send_audio_opus(encoded_audio.data(), encoded_audio_length);
	return *this;
}

discord_voice_client& discord_voice_client::send_audio_opus(const uint8_t* opus_packet, const size_t length) {
	int samples = opus_packet_get_nb_samples(opus_packet, (opus_int32)length, opus_sample_rate_hz);
	uint64_t duration = (samples / 48) / (timescale / 1000000);
	send_audio_opus(opus_packet, length, duration, false);
	return *this;
}

discord_voice_client& discord_voice_client::send_audio_opus(const uint8_t* opus_packet, const size_t length, uint64_t duration, bool send_now) {
	int frame_size = (int)(48 * duration * (timescale / 1000000));
	opus_int32 encoded_audio_max_length = (opus_int32)length;
	std::vector<uint8_t> encoded_audio(encoded_audio_max_length);
	size_t encoded_audio_length = encoded_audio_max_length;

	encoded_audio_length = length;
	encoded_audio.reserve(length);
	memcpy(encoded_audio.data(), opus_packet, length);

	if (this->is_end_to_end_encrypted()) {

		std::vector<uint8_t> encrypted_buffer(this->mls_state->encryptor->get_max_ciphertext_byte_size(dave::media_type::media_audio, length));
		size_t out_size{0};

		auto result = this->mls_state->encryptor->encrypt(
			dave::media_type::media_audio,
			ssrc,
			dave::make_array_view<const uint8_t>(encoded_audio.data(), length),
			dave::make_array_view(encrypted_buffer),
			&out_size
		);
		encrypted_buffer.resize(out_size);
		if (result != dave::encryptor::result_code::rc_success) {
			log(ll_warning, "DAVE Encryption failure: " + std::to_string(result));
		} else {
			encoded_audio = encrypted_buffer;
			encoded_audio_length = encrypted_buffer.size();
		}
	}

	++sequence;
	rtp_header header(sequence, timestamp, (uint32_t)ssrc);

	/* Expected payload size is unencrypted header + encrypted opus packet + unencrypted 32 bit nonce */
	size_t packet_siz = sizeof(header) + (encoded_audio_length + ssl_crypto_aead_xchacha20poly1305_IETF_ABYTES) + sizeof(packet_nonce);

	std::vector<uint8_t> payload(packet_siz);

	/* Set RTP header */
	std::memcpy(payload.data(), &header, sizeof(header));

	/* Convert nonce to big-endian */
	uint32_t noncel = htonl(packet_nonce);

	/* 24 byte is needed for encrypting, discord just want 4 byte so just fill up the rest with null */
	unsigned char encrypt_nonce[ssl_crypto_aead_xchacha20poly1305_ietf_NPUBBYTES] = { '\0' };
	memcpy(encrypt_nonce, &noncel, sizeof(noncel));

	  /* Execute */
	unsigned long long int clen{0};
	if (ssl_crypto_aead_xchacha20poly1305_ietf_encrypt(
		payload.data() + sizeof(header),
		&clen,
		encoded_audio.data(),
		encoded_audio_length,
		/* The RTP Header as Additional Data */
		reinterpret_cast<const unsigned char *>(&header),
		sizeof(header),
		nullptr,
		static_cast<const unsigned char*>(encrypt_nonce),
		secret_key.data()
	) != 0) {
		log(dpp::ll_debug, "XChaCha20 Encryption failed");
	}

	/* Append the 4 byte nonce to the resulting payload */
	std::memcpy(payload.data() + payload.size() - sizeof(noncel), &noncel, sizeof(noncel));

	this->send(reinterpret_cast<const char *>(payload.data()), payload.size(), duration, send_now);

	timestamp += frame_size;

	/* Increment for next packet */
	packet_nonce++;

	speak();
	return *this;
}

size_t discord_voice_client::encode(uint8_t *input, size_t inDataSize, uint8_t *output, size_t &outDataSize) {
	outDataSize = 0;
	int mEncFrameBytes = 11520;
	int mEncFrameSize = 2880;
	if (0 == (inDataSize % mEncFrameBytes)) {
		bool isOk = true;
		uint8_t *out = encode_buffer;

		memset(out, 0, sizeof(encode_buffer));
		repacketizer = opus_repacketizer_init(repacketizer);
		if (!repacketizer) {
			log(ll_warning, "opus_repacketizer_init(): failure");
			return outDataSize;
		}
		for (size_t i = 0; i < (inDataSize / mEncFrameBytes); ++ i) {
			const opus_int16* pcm = reinterpret_cast<opus_int16*>(input + i * mEncFrameBytes);
			int ret = opus_encode(encoder, pcm, mEncFrameSize, out, 65536);
			if (ret > 0) {
				int retval = opus_repacketizer_cat(repacketizer, out, ret);
				if (retval != OPUS_OK) {
					isOk = false;
					log(ll_warning, "opus_repacketizer_cat(): " + std::string(opus_strerror(retval)));
					break;
				}
				out += ret;
			} else {
				isOk = false;
				log(ll_warning, "opus_encode(): " + std::string(opus_strerror(ret)));
				break;
			}
		}
		if (isOk) {
			int ret = opus_repacketizer_out(repacketizer, output, 65536);
			if (ret > 0) {
				outDataSize = ret;
			} else {
				log(ll_warning, "opus_repacketizer_out(): " + std::string(opus_strerror(ret)));
			}
		}
	} else {
		throw dpp::voice_exception(err_invalid_voice_packet_length, "Invalid input data length: " + std::to_string(inDataSize) + ", must be n times of " + std::to_string(mEncFrameBytes));
	}
	return outDataSize;
}


}
