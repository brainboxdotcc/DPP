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
#include <dpp/utility.h>
#include <dpp/stringops.h>
#include <dpp/exception.h>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <thread>
#include <functional>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <fstream>
#include <streambuf>
#include <array>
#include <dpp/fmt/format.h>
#include <dpp/dispatcher.h>

#ifdef _WIN32
	#include <stdio.h>
	#include <stdlib.h>
	#define popen _popen
	#define pclose _pclose
#endif

using namespace std::literals;

namespace dpp {

	namespace utility {

		double time_f()
		{
			using namespace std::chrono;
			auto tp = system_clock::now() + 0ns;
			return tp.time_since_epoch().count() / 1000000000.0;
		}

		bool has_voice() {
#if HAVE_VOICE
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
			if (hours == 0 && days == 0) {
				return fmt::format("{:02d}:{:02d}", mins, secs);
			} else {
				return fmt::format("{}{:02d}:{:02d}:{:02d}", (days ? fmt::format("{} day{}, ", days, (days > 1 ? "s" : "")) : ""), hours, mins, secs);
			}
		}

		uint64_t uptime::to_secs() {
			return secs + (mins * 60) + (hours * 60 * 60) + (days * 60 * 60 * 24);
		}

		uint64_t uptime::to_msecs() {
			return to_secs() * 1000;
		}

		iconhash::iconhash() : first(0), second(0) {
		}

		void iconhash::set(const std::string &hash) {
			std::string clean_hash(hash);
			if (hash.empty()) {	// Clear values if empty hash
				first = second = 0;
				return;
			}
			if (hash.length() == 34 && hash.substr(0, 2) == "a_") {
				/* Someone passed in an animated icon. numpty.
				 * Clean that mess up!
				 */
				clean_hash = hash.substr(2);
			}
			if (clean_hash.length() != 32)
				throw std::length_error("iconhash must be exactly 32 characters in length, passed value is: '" + clean_hash + "'");
			this->first = from_string<uint64_t>(clean_hash.substr(0, 16), std::hex);
			this->second = from_string<uint64_t>(clean_hash.substr(16, 16), std::hex);
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

		std::string debug_dump(uint8_t* data, size_t length) {
			std::ostringstream out;
			size_t addr = (size_t)data;
			size_t extra = addr % 16;
			if (extra != 0) {
				addr -= extra;
				out << fmt::format("[{:016X}] : ", addr);
			}
			for (size_t n = 0; n < extra; ++n) {
				out << "-- ";
			}
			std::string ascii;
			for (uint8_t* ptr = data; ptr < data + length; ++ptr) {
				if (((size_t)ptr % 16) == 0) {
					out << fmt::format("    {}\n[{:016X}] : ", ascii, (size_t)ptr);
					ascii.clear();
				}
				ascii.push_back(*ptr >= ' ' && *ptr <= '~' ? *ptr : '.');
				out << fmt::format("{:02X} ", *ptr);
			}
			out << "    " << ascii;
			out << "\n";
			return out.str();
		}

		std::string bytes(uint64_t c) {
			if (c > 1099511627776) {	// 1TB
				return fmt::format("{:.2f}T", (c / 1099511627776.0));
			} else if (c > 1073741824) {	// 1GB
				return fmt::format("{:.2f}G", (c / 1073741824.0));
			}  else if (c > 1048576) {	// 1MB
				return fmt::format("{:.2f}M", (c / 1048576.0));
			}  else if (c > 1024) {		// 1KB
				return fmt::format("{:.2f}K", (c / 1024.0));
			} else {			// Bytes
				return std::to_string(c);
			}
		}

		uint32_t rgb(float red, float green, float blue) {
			return (((uint32_t)(red * 255)) << 16) | (((uint32_t)(green * 255)) << 8) | ((uint32_t)(blue * 255));
		}

		/* NOTE: Parameters here are `int` instead of `uint32_t` or `uint8_t` to prevent ambiguity error with rgb(float, float, float) */
		uint32_t rgb(int red, int green, int blue) {
			return ((uint32_t)red << 16) | ((uint32_t)green << 8) | (uint32_t)blue;
		}

		void exec(const std::string& cmd, std::vector<std::string> parameters, cmd_result_t callback) {
			auto t = std::thread([cmd, parameters, callback]() {
				std::array<char, 128> buffer;
				std::vector<std::string> my_parameters = parameters;
				std::string result;
				std::stringstream cmd_and_parameters;
				cmd_and_parameters << cmd;
				for (auto & parameter : my_parameters) {
					cmd_and_parameters << " " << std::quoted(parameter);
				}
				/* Capture stderr */
				cmd_and_parameters << " 2>&1";
				std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd_and_parameters.str().c_str(), "r"), pclose);
				if (!pipe) {
					return;
				}
				while (fgets(buffer.data(), (int)buffer.size(), pipe.get()) != nullptr) {
					result += buffer.data();
				}
				if (callback) {
					callback(result);
				}
			});
			t.detach();
		}

		size_t utf8len(const std::string &str)
		{
			size_t i = 0, iBefore = 0, count = 0;
			const char* s = str.c_str();
			if (*s == 0)
				return 0;

			while (s[i] > 0) {
		ascii:
				i++;
			}

			count += i - iBefore;

			while (s[i]) {
				if (s[i] > 0) {
					iBefore = i;
					goto ascii;
				} else {
					switch (0xF0 & s[i]) {
					case 0xE0:
						i += 3;
						break;
					case 0xF0:
						i += 4;
						break;
					default:
						i += 2;
						break;
					}
				}

				count++;
			}

			return count;
		}

		std::string utf8substr(const std::string& str, std::string::size_type start, std::string::size_type leng)
		{
			if (leng == 0) {
				return "";
			}
			if (start == 0 && leng >= utf8len(str)) {
				return str;
			}
			std::string::size_type i, ix, q, min = std::string::npos, max = std::string::npos;
			for (q = 0, i = 0, ix = str.length(); i < ix; i++, q++)
			{
				if (q == start)
					min = i;
				if (q <= start + leng || leng == std::string::npos)
					max = i;

				unsigned char c = (unsigned char)str[i];
				if (c < 0x80)
					i += 0;
				else if ((c & 0xE0) == 0xC0)
					i += 1;
				else if ((c & 0xF0) == 0xE0)
					i += 2;
				else if ((c & 0xF8) == 0xF0)
					i += 3;
				else
					return "";	//invalid utf8
			}
			if (q <= start + leng || leng == std::string::npos)
				max = i;
			if (min == std::string::npos || max == std::string::npos)
				return "";

			return str.substr(min, max);
		}

		std::string read_file(const std::string& filename)
		{
			try {
				std::ifstream ifs(filename, std::ios::binary);
				return std::string((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
			}
			catch (const std::exception& e) {
				/* Rethrow as dpp::file_exception */
				throw dpp::file_exception(e.what());
			}
		}

		std::string validate(const std::string& value, size_t _min, size_t _max, const std::string& exception_message) {
			if (utf8len(value) < _min) {
				throw dpp::length_exception(exception_message);
			} else if (utf8len(value) > _max) {
				return utf8substr(value, 0, _max);
			}
			return value;
		}


		std::string timestamp(time_t ts, time_format tf) {
			char format[2] = { (char)tf, 0 };
			return "<t:" + std::to_string(ts) + ":" + format + ">";
		}

		std::string avatar_size(uint32_t size) {
			if (size) {
				return "?size=" + std::to_string(size);
			}
			return std::string();
		}

		std::vector<std::string> tokenize(std::string const &in, const char* sep) {
			std::string::size_type b = 0;
			std::vector<std::string> result;

			while ((b = in.find_first_not_of(sep, b)) != std::string::npos) {
				auto e = in.find(sep, b);
				result.push_back(in.substr(b, e-b));
				b = e;
			}
			return result;
		}

		std::string bot_invite_url(const snowflake bot_id, const uint64_t permissions, const std::vector<std::string>& scopes) {
			return fmt::format("https://discord.com/oauth2/authorize?client_id={}&permissions={}&scope={}",
				bot_id,
				permissions,
				fmt::join(scopes, "+")
			);
		}

		std::function<void(const dpp::log_t&)> cout_logger() {
			return [](const dpp::log_t& event) {
				if (event.severity > dpp::ll_trace) {
					std::cout << dpp::utility::loglevel(event.severity) << ": " << event.message << "\n";
				}
			};
		}

	};

};
