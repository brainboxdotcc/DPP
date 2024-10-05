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

#include <memory>

#include "common.h"
#include "array_view.h"

namespace dpp::dave {

using const_byte_view = array_view<const uint8_t>;
using byte_view = array_view<uint8_t>;

class cipher_interface { // NOLINT
public:
	virtual ~cipher_interface() = default;

	virtual bool encrypt(byte_view ciphertextBufferOut, const_byte_view plaintextBuffer, const_byte_view nonceBuffer, const_byte_view additionalData, byte_view tagBufferOut) = 0;
	virtual bool decrypt(byte_view plaintextBufferOut, const_byte_view ciphertextBuffer, const_byte_view tagBuffer, const_byte_view nonceBuffer, const_byte_view additionalData) = 0;
};

std::unique_ptr<cipher_interface> create_cipher(const encryption_key& encryptionKey);

} // namespace dpp::dave

