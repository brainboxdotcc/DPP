/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
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
#include <map>
#include <dpp/discord.h>
#include <dpp/cluster.h>
#include <dpp/discordclient.h>
#include <dpp/discordevents.h>
#include <dpp/message.h>
#include <dpp/cache.h>
#include <dpp/nlohmann/json.hpp>
#include <utility>

namespace dpp {

void cluster::direct_message_create(snowflake user_id, const message &m, command_completion_event_t callback) {
	/* Find out if a DM channel already exists */
	message msg = m;
	snowflake dm_channel_id = this->get_dm_channel(user_id);
	if (!dm_channel_id) {
		this->create_dm_channel(user_id, [user_id, this, msg, callback](const dpp::confirmation_callback_t& completion) {
			/* NOTE: We are making copies in here for a REASON. Don't try and optimise out these
			 * copies as if we use references, by the time the the thread completes for the callback
			 * the reference is invalid and we get a crash or heap corruption!
			 */
			message m2 = msg;
			dpp::channel c = std::get<channel>(completion.value);
			m2.channel_id = c.id;
			this->set_dm_channel(user_id, c.id);
			message_create(m2, callback);
		});
	} else {
		msg.channel_id = dm_channel_id;
		message_create(msg, callback);
	}
}

};