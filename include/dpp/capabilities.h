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
#pragma once

namespace dpp {

/**
 * @brief capabilities are a bitmask where you can opt a bot client into gateway behaviors.
 * 
 * This is a separate Identify bitfield from intents,
 * Intents control which events your bot client receives,
 * while capabilities affects gateway behaviors.
 */
enum capabilities {
	/**
	 * @brief Opts the client into receiving obfuscated channel metadata over the Gateway for channels it can’t view.
	 */
	c_channel_obfuscation		= (1 << 15),

	/**
	 * @brief Default D++ capabilities
	 */
	c_default_capabilities		= c_channel_obfuscation
};

}
