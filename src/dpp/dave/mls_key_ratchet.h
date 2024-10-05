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

#include <mls/key_schedule.h>
#include "key_ratchet.h"

namespace dpp::dave {

class mls_key_ratchet : public key_ratchet_interface {
public:
	mls_key_ratchet(::mlspp::CipherSuite suite, bytes baseSecret) noexcept;
	~mls_key_ratchet() noexcept override;

	encryption_key get_key(key_generation generation) noexcept override;
	void delete_key(key_generation generation) noexcept override;

private:
	::mlspp::HashRatchet hashRatchet_;
};

} // namespace dpp::dave

