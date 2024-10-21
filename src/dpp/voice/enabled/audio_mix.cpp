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
#include <algorithm>
#include <cmath>
#include <dpp/exception.h>
#include <dpp/isa_detection.h>
#include <dpp/discordvoiceclient.h>
#include <opus/opus.h>

namespace dpp {

size_t audio_mix(discord_voice_client &client, audio_mixer &mixer, opus_int32 *pcm_mix, const opus_int16 *pcm, size_t park_count, int samples, int &max_samples) {
	/* Mix the combined stream if combined audio is bound */
	if (client.creator->on_voice_receive_combined.empty()) {
		return 0;
	}

	/* We must upsample the data to 32 bits wide, otherwise we could overflow */
	for (opus_int32 v = 0; v < (samples * opus_channel_count) / mixer.byte_blocks_per_register; ++v) {
		mixer.combine_samples(pcm_mix, pcm);
		pcm += mixer.byte_blocks_per_register;
		pcm_mix += mixer.byte_blocks_per_register;
	}
	client.moving_average += park_count;
	max_samples = (std::max)(samples, max_samples);
	return park_count + 1;
}

};
