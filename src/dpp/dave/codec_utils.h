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

#include "common.h"
#include "frame_processors.h"
#include "array_view.h"

/**
 * @brief Functions for processing specific frame types.
 * Different types of audio/video frames have different rules for what parts of it
 * must remain unencrypted to allow for routing and processing further up the chain.
 */
namespace dpp::dave::codec_utils {

/**
 * @brief process opus audio frame
 * @param processor outbound frame processor
 * @param frame frame bytes
 * @return true if frame could be processed
 */
bool process_frame_opus(outbound_frame_processor & processor, array_view<const uint8_t> frame);

/**
 * @brief process VP8 video frame
 * @param processor outbound frame processor
 * @param frame frame bytes
 * @return true if frame could be processed
 */
bool process_frame_vp8(outbound_frame_processor & processor, array_view<const uint8_t> frame);

/**
 * @brief process VP9 video frame
 * @param processor outbound frame processor
 * @param frame frame bytes
 * @return true if frame could be processed
 */
bool process_frame_vp9(outbound_frame_processor & processor, array_view<const uint8_t> frame);

/**
 * @brief process H264 video frame
 * @param processor outbound frame processor
 * @param frame frame bytes
 * @return true if frame could be processed
 */
bool process_frame_h264(outbound_frame_processor & processor, array_view<const uint8_t> frame);

/**
 * @brief process opus frame
 * @param processor outbound frame processor
 * @param frame frame bytes
 * @return true if frame could be processed
 */
bool process_frame_h265(outbound_frame_processor & processor, array_view<const uint8_t> frame);

/**
 * @brief process opus frame
 * @param processor outbound frame processor
 * @param frame frame bytes
 * @return true if frame could be processed
 */
bool process_frame_av1(outbound_frame_processor & processor, array_view<const uint8_t> frame);

/**
 * @brief Check if encrypted frame is valid
 * @param processor outbound frame processor
 * @param frame frame to validate
 * @return true if frame could be validated
 */
bool validate_encrypted_frame(outbound_frame_processor& processor, array_view<uint8_t> frame);

}
