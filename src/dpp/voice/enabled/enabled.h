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
#pragma once

#include <dpp/export.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <fcntl.h>
#include <csignal>
#include <cstring>
#include <string>
#include <map>
#include <vector>
#include <dpp/json_fwd.h>
#include <dpp/wsclient.h>
#include <dpp/dispatcher.h>
#include <dpp/cluster.h>
#include <dpp/discordevents.h>
#include <dpp/socket.h>
#include <dpp/isa_detection.h>
#include <queue>
#include <thread>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <future>
#include <functional>
#include <chrono>
#include <iostream>
#include <set>
#include <dpp/discordvoiceclient.h>
#include <sodium.h>
#include <opus/opus.h>
#include "../../dave/session.h"
#include "../../dave/decryptor.h"
#include "../../dave/encryptor.h"

#ifdef _WIN32
#include <WinSock2.h>
	#include <WS2tcpip.h>
	#include <io.h>
#else
	#include <arpa/inet.h>
	#include <netinet/in.h>
	#include <sys/socket.h>
#endif

namespace dpp {

/**
 * @brief A list of MLS decryptors for decrypting inbound audio from users by snowflake id
 */
using decryptor_list = std::map<snowflake, std::unique_ptr<dave::decryptor>>;

/**
 * @brief Holds all internal DAVE E2EE encryption state
 */
struct dave_state {
	/**
	 * @brief libdave session
	 */
	std::unique_ptr<dave::mls::session> dave_session{};
	/**
	 * @brief Our key package
	 */
	std::shared_ptr<::mlspp::SignaturePrivateKey> mls_key;
	/**
	 * @brief Cached commit package for use in welcome
	 */
	std::vector<uint8_t> cached_commit;
	/**
	 * @brief Current transition ID
	 */
	uint64_t transition_id{0};
	/**
	 * @brief Details of upcoming transition
	 */
	struct {
		/**
		 * @brief pending next transition ID
		 */
		uint64_t id{0};
		/**
		 * @brief New upcoming protocol version
		 */
		uint64_t protocol_version{0};
		/**
		 * @brief True if transition is pending
		 */
		bool is_pending{false};
	} pending_transition;
	/**
	 * @brief Decryptors for inbound audio streams
	 */
	decryptor_list decryptors;
	/**
	 * @brief Encryptor for outbound audio stream
	 */
	std::unique_ptr<dave::encryptor> encryptor;
	/**
	 * @brief Current privacy code, or empty string if
	 * MLS group is not established.
	 */
	std::string privacy_code;
};

/**
 * @brief Represents an RTP packet. Size should always be exactly 12.
 */
struct rtp_header {
	uint16_t constant;
	uint16_t sequence;
	uint32_t timestamp;
	uint32_t ssrc;

	rtp_header(uint16_t _seq, uint32_t _ts, uint32_t _ssrc) : constant(htons(0x8078)), sequence(htons(_seq)), timestamp(htonl(_ts)), ssrc(htonl(_ssrc)) {
	}
};

/**
* @brief Transport encryption type (libsodium)
*/
constexpr std::string_view transport_encryption_protocol = "aead_xchacha20_poly1305_rtpsize";

std::string generate_displayable_code(const std::vector<uint8_t> &data, size_t desired_length = 30, size_t group_size = 5);

size_t audio_mix(discord_voice_client &client, audio_mixer &mixer, opus_int32 *pcm_mix, const opus_int16 *pcm, size_t park_count, int samples, int &max_samples);

}
