#include <dpp/discord.h>
#include <ctime>
#include <iomanip>
#include <sstream>

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
				case dpp::ll_critical: return "CRIT";
				default: return "???";
			}
		}

	};

};
