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
#include "leb128.h"

namespace dpp::dave {

size_t leb128_size(uint64_t value)
{
	int size = 0;
	while (value >= 0x80) {
		++size;
		value >>= 7;
	}
	return size + 1;
}

uint64_t read_leb128(const uint8_t*& readAt, const uint8_t* end)
{
	uint64_t value = 0;
	int fillBits = 0;
	while (readAt != end && fillBits < 64 - 7) {
		uint8_t leb128Byte = *readAt;
		value |= uint64_t{leb128Byte & 0x7Fu} << fillBits;
		++readAt;
		fillBits += 7;
		if ((leb128Byte & 0x80) == 0) {
			return value;
		}
	}
	// Read 9 bytes and didn't find the terminator byte. Check if 10th byte
	// is that terminator, however to fit result into uint64_t it may carry only
	// single bit.
	if (readAt != end && *readAt <= 1) {
		value |= uint64_t{*readAt} << fillBits;
		++readAt;
		return value;
	}
	// Failed to find terminator leb128 byte.
	readAt = nullptr;
	return 0;
}

size_t write_leb128(uint64_t value, uint8_t* buffer)
{
	int size = 0;
	while (value >= 0x80) {
		buffer[size] = 0x80 | (value & 0x7F);
		++size;
		value >>= 7;
	}
	buffer[size] = value;
	++size;
	return size;
}

} // namespace dpp::dave

