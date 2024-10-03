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

namespace dpp::dave::codec_utils {

bool process_frame_opus(OutboundFrameProcessor & processor, array_view<const uint8_t> frame);
bool process_frame_vp8(OutboundFrameProcessor & processor, array_view<const uint8_t> frame);
bool process_frame_vp9(OutboundFrameProcessor & processor, array_view<const uint8_t> frame);
bool process_frame_h264(OutboundFrameProcessor & processor, array_view<const uint8_t> frame);
bool process_frame_h265(OutboundFrameProcessor & processor, array_view<const uint8_t> frame);
bool process_frame_av1(OutboundFrameProcessor & processor, array_view<const uint8_t> frame);

bool validate_encrypted_frame(OutboundFrameProcessor& processor, array_view<uint8_t> frame);

} // namespace dpp::dave::codec_utils


