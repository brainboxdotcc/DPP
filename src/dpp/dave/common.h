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
 * This folder is a modified fork of libdave, https://github.com/discord/libdave
 * Copyright (c) 2024 Discord, Licensed under MIT
 *
 ************************************************************************************/
#pragma once

#include <array>
#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "version.h"

namespace mlspp::bytes_ns {
	/**
	 * @brief bytes type
	 */
	struct bytes;
};

namespace dpp::dave {

/**
 * @brief Unencrypted frame header size
 */
using unencrypted_frame_header_size = uint16_t;

/**
 * @brief Truncated sync nonce
 */
using truncated_sync_nonce = uint32_t;

/**
 * @brief Magic marker
 */
using magic_marker = uint16_t;

/**
 * @brief Encryption key
 */
using encryption_key = ::mlspp::bytes_ns::bytes;

/**
 * @brief Transition ID
 */
using transition_id = uint16_t;

/**
 * @brief Supplemental bytes size
 */
using supplemental_bytes_size = uint8_t;

/**
 * @brief Media frame types
 */
enum media_type : uint8_t { media_audio, media_video };


/**
 * @brief Media codec types
 */
enum codec : uint8_t { cd_unknown, cd_opus, cd_vp8, cd_vp9, cd_h264, cd_h265, cd_av1 };

/**
 * @brief Returned in std::variant when a message is hard-rejected and should trigger a reset
 */
struct failed_t {};

/**
 * @brief Returned in std::variant when a message is soft-rejected and should not trigger a reset
 */
struct ignored_t {};

/**
 * @briefMap of ID-key pairs.
 * In process_commit, this lists IDs whose keys have been added, changed, or removed;
 * an empty value value means a key was removed.
 */
using roster_map = std::map<uint64_t, std::vector<uint8_t>>;

/**
 * @brief Return type for functions producing RosterMap or hard or soft failures
 */
using roster_variant = std::variant<failed_t, ignored_t, roster_map>;

/**
 * @brief Magic marker ID
 */
constexpr magic_marker MARKER_BYTES = 0xFAFA;

/**
 * Layout constants
 */
constexpr size_t AES_GCM_128_KEY_BYTES = 16;
constexpr size_t AES_GCM_128_NONCE_BYTES = 12;
constexpr size_t AES_GCM_128_TRUNCATED_SYNC_NONCE_BYTES = 4;
constexpr size_t AES_GCM_128_TRUNCATED_SYNC_NONCE_OFFSET = AES_GCM_128_NONCE_BYTES - AES_GCM_128_TRUNCATED_SYNC_NONCE_BYTES;
constexpr size_t AES_GCM_127_TRUNCATED_TAG_BYTES = 8;
constexpr size_t RATCHET_GENERATION_BYTES = 1;
constexpr size_t RATCHET_GENERATION_SHIFT_BITS = 8 * (AES_GCM_128_TRUNCATED_SYNC_NONCE_BYTES - RATCHET_GENERATION_BYTES);
constexpr size_t SUPPLEMENTAL_BYTES = AES_GCM_127_TRUNCATED_TAG_BYTES + sizeof(supplemental_bytes_size) + sizeof(magic_marker);
constexpr size_t TRANSFORM_PADDING_BYTES = 64;

/**
 * Timing constants
 */
constexpr auto DEFAULT_TRANSITION_EXPIRY = std::chrono::seconds(10);
constexpr auto CIPHER_EXPIRY = std::chrono::seconds(10);

/**
 * Behavior constants
 */
constexpr auto INIT_TRANSITION_ID = 0;
constexpr auto DISABLED_VERSION = 0;
constexpr auto MAX_GENERATION_GAP = 250;
constexpr auto MAX_MISSING_NONCES = 1000;
constexpr auto GENERATION_WRAP = 1 << (8 * RATCHET_GENERATION_BYTES);
constexpr auto MAX_FRAMES_PER_SECOND = 50 + 2 * 60; // 50 audio frames + 2 * 60fps video streams
constexpr std::array<uint8_t, 3> OPUS_SILENCE_PACKET = {0xF8, 0xFF, 0xFE};

// Utility routine for variant return types

/**
 * @brief Utility to get variant return types wrapped in an optional
 * @tparam T type to get from variant
 * @tparam V type for the optional
 * @param variant variant to get from
 * @return value retrieved or nullopt if not found
 */
template <class T, class V> inline std::optional<T> get_optional(V&& variant)
{
	if (auto map = std::get_if<T>(&variant)) {
		if constexpr (std::is_rvalue_reference_v<decltype(variant)>) {
			return std::move(*map);
		} else {
			return *map;
		}
	}
	else {
		return std::nullopt;
	}
}

} // namespace dpp::dave

