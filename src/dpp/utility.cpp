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
#include <dpp/discord.h>
#include <dpp/stringops.h>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fmt/format.h>

namespace dpp {

	namespace utility {

		bool has_voice() {
#if HAS_VOICE
			return true;
#else
			return false;
#endif
		}

		std::string current_date_time() {
#ifdef _WIN32
			std::time_t curr_time = time(nullptr);
			return std::ctime(&curr_time);
#else
			auto t = std::time(nullptr);
			struct tm timedata;
			localtime_r(&t, &timedata);
			std::stringstream s;
			s << std::put_time(&timedata, "%Y-%m-%d %H:%M:%S");
			return s.str();
#endif
		}

		std::string loglevel(dpp::loglevel in) {
			switch (in) {
				case dpp::ll_trace: return "TRACE";
				case dpp::ll_debug: return "DEBUG";
				case dpp::ll_info: return "INFO";
				case dpp::ll_warning: return "WARN";
				case dpp::ll_error: return "ERROR";
				case dpp::ll_critical: return "CRIT";
				default: return "???";
			}
		}

		uptime::uptime() : days(0), hours(0), mins(0), secs(0) {
		}

		uptime::uptime(time_t diff) : uptime() {
			days = (uint16_t)(diff / (3600 * 24));
			hours = (uint8_t)(diff % (3600 * 24) / 3600);
			mins = (uint8_t)(diff % 3600 / 60);
			secs = (uint8_t)(diff % 60);
		}

		std::string uptime::to_string() {
			return fmt::format("{}{:02d}:{:02d}:{:02d}", (days ? fmt::format("{} day{}, ", days, (days > 1 ? "s" : "")) : ""), hours, mins, secs);
		}

		iconhash::iconhash() : first(0), second(0) {
		}

		void iconhash::set(const std::string &hash) {
			if (hash.empty()) {	// Clear values if empty hash
				first = second = 0;
				return;
			}
			if (hash.length() != 32)
				throw std::length_error("iconhash must be exactly 32 characters in length");
			this->first = from_string<uint64_t>(hash.substr(0, 16), std::hex);
			this->second = from_string<uint64_t>(hash.substr(16, 16), std::hex);
		}

		iconhash::iconhash(const std::string &hash) {
			set(hash);
		}

		iconhash& iconhash::operator=(const std::string &assignment) {
			set(assignment);
			return *this;
		}

		std::string iconhash::to_string() const {
			if (first == 0 && second == 0)
				return "";
			else
				return fmt::format("{:016x}{:016x}", this->first, this->second);
		}

		void debug_dump(uint8_t* data, size_t length) {
			size_t addr = (size_t)data;
			size_t extra = addr % 16;
			if (extra != 0) {
				addr -= extra;
				std::cout << fmt::format("[{:016X}] : ", addr);
			}
			for (size_t n = 0; n < extra; ++n) {
				std::cout << "-- ";
			}
			for (uint8_t* ptr = data; ptr < data + length; ++ptr) {
				if (((size_t)ptr % 16) == 0) {
					std::cout << fmt::format("\n[{:016X}] : ", (size_t)ptr);
				}
				std::cout << fmt::format("{:02X} ", *ptr);
			}
			std::cout << "\n";
		}

	};

};
