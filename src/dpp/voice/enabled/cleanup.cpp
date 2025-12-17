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
#include <fstream>
#include <dpp/exception.h>
#include <dpp/isa_detection.h>
#include <dpp/discordvoiceclient.h>
#include <opus/opus.h>
#include "../../dave/encryptor.h"
#include "enabled.h"

namespace dpp {

void discord_voice_client::cleanup()
{
	if (encoder != nullptr) {
		opus_encoder_destroy(encoder);
		encoder = nullptr;
	}
	if (repacketizer != nullptr) {
		opus_repacketizer_destroy(repacketizer);
		repacketizer = nullptr;
	}
	if (voice_courier.joinable()) {
		{
			std::lock_guard lk(voice_courier_shared_state.mtx);
			voice_courier_shared_state.terminating = true;
		}
		voice_courier_shared_state.signal_iteration.notify_one();
		voice_courier.join();
	}
	if (fd != INVALID_SOCKET) {
		owner->socketengine->delete_socket(fd);
	}
}

}
