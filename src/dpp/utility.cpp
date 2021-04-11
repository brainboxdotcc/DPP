#include <dpp/discord.h>
#include <dpp/stringops.h>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fmt/format.h>

namespace dpp {

	namespace utility {

		std::string current_date_time() {
			auto t = std::time(nullptr);
			struct tm timedata;
			localtime_r(&t, &timedata);
			std::stringstream s;
			s << std::put_time(&timedata, "%Y-%m-%d %H:%M:%S");
			return s.str();
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
			days = (diff / (3600 * 24));
			hours = (diff % (3600 * 24) / 3600);
			mins = (diff % 3600 / 60);
			secs = (diff % 60);
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

	};

};
