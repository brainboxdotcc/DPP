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

#include <fstream>
#include <dpp/exception.h>
#include <dpp/isa_detection.h>
#include <dpp/discordvoiceclient.h>

namespace dpp {

bool discord_voice_client::voice_payload::operator<(const voice_payload& other) const {
	if (timestamp != other.timestamp) {
		return timestamp > other.timestamp;
	}

	constexpr rtp_seq_t wrap_around_test_boundary = 5000;
	if ((seq < wrap_around_test_boundary && other.seq >= wrap_around_test_boundary)
	    || (seq >= wrap_around_test_boundary && other.seq < wrap_around_test_boundary)) {
		/* Match the cases where exactly one of the sequence numbers "may have"
		 * wrapped around.
		 *
		 * Examples:
		 * 1. this->seq = 65530, other.seq = 10  // Did wrap around
		 * 2. this->seq = 5002, other.seq = 4990 // Not wrapped around
		 *
		 * Add 5000 to both sequence numbers to force wrap around so they can be
		 * compared. This should be fine to do to case 2 as well, as long as the
		 * addend (5000) is not too large to cause one of them to wrap around.
		 *
		 * In practice, we should be unlikely to hit the case where
		 *
		 *           this->seq = 65530, other.seq = 5001
		 *
		 * because we shouldn't receive more than 5000 payloads in one batch, unless
		 * the voice courier thread is super slow. Also remember that the timestamp
		 * is compared first, and payloads this far apart shouldn't have the same
		 * timestamp.
		 */

		/* Casts here ensure the sum wraps around and not implicitly converted to
		 * wider types.
		 */
		return   static_cast<rtp_seq_t>(seq + wrap_around_test_boundary)
			 > static_cast<rtp_seq_t>(other.seq + wrap_around_test_boundary);
	} else {
		return seq > other.seq;
	}
}

}
