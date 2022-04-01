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
#pragma once

#include <cstdint>

 /**
  * @brief The main namespace for D++ functions. classes and types
 */
namespace dpp {
	/**
     * @brief predefined color constants
    */
	namespace colors {
		const uint32_t
			white = 0xFFFFFF,
			discord_white = 0xFFFFFE,
			light_gray = 0xC0C0C0,
			gray = 0x808080,
			dark_gray = 0x404040,
			black = 0x000000,
			discord_black = 0x000001,
			red = 0xFF0000,
			pink = 0xFFAFAF,
			orange = 0xFFC800,
			yellow = 0xFFFF00,
			green = 0x00FF00,
			magenta = 0xFF00FF,
			cyan = 0x00FFFF,
			blue = 0x0000FF,
			light_sea_green = 0x1ABC9C,
			medium_sea_green = 0x2ECC71,
			summer_sky = 0x3498DB,
			deep_lilac = 0x9B59B6,
			ruby = 0xE91E63,
			moon_yellow = 0xF1C40F,
			tahiti_gold = 0xE67E22,
			cinnabar = 0xE74C3C,
			submarine = 0x95A5A6,
			blue_aquamarine = 0x607D8B,
			deep_sea = 0x11806A,
			sea_green = 0x1F8B4C,
			endeavour = 0x206694,
			vivid_violet = 0x71368A,
			jazzberry_jam = 0xAD1457,
			dark_goldenrod = 0xC27C0E,
			rust = 0xA84300,
			brown = 0x992D22,
			gray_chateau = 0x979C9F,
			bismark = 0x546E7A,
			sti_blue = 0x0E4BEF,
			wrx_blue = 0x00247D,
			rallyart_crimson = 0xE60012,
			lime = 0x00FF00,
			forest_green = 0x228B22,
			cadmium_green = 0x097969,
			aquamarine = 0x7FFFD4,
			blue_green = 0x088F8F,
			raspberry = 0xE30B5C,
			scarlet_red = 0xFF2400;
	};
};
