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
 *  https://webrtc.googlesource.com/src/+/refs/heads/main/modules/rtp_rtcp/source/leb128.cc
 *  Copyright (c) 2023 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 *
 ************************************************************************************/
#pragma once

#include <cstddef>
#include <cstdint>

namespace dpp::dave {

/**
 * @brief Maximum size of LEB128 value
 */
constexpr size_t LEB128_MAX_SIZE = 10;

/**
 * @brief Returns number of bytes needed to store `value` in leb128 format.
 * @param value value to return size for
 * @return size of leb128
 */
size_t leb128_size(uint64_t value);

/**
 * @brief Reads leb128 encoded value and advance read_at by number of bytes consumed.
 * Sets read_at to nullptr on error.
 * @param read_at start position
 * @param end end position
 * @return decoded value
 */
uint64_t read_leb128(const uint8_t*& read_at, const uint8_t* end);

/**
 * @brief Encodes `value` in leb128 format. Assumes buffer has size of
 * at least Leb128Size(value). Returns number of bytes consumed.
 * @param value value to encode
 * @param buffer buffer to encode into
 * @return size of encoding
 */
size_t write_leb128(uint64_t value, uint8_t* buffer);

}
