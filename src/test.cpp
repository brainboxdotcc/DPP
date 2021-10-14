#undef DPP_BUILD
#ifdef _WIN32
_Pragma("warning( disable : 4251 )"); // 4251 warns when we export classes or structures with stl member variables
#endif
#include <dpp/dpp.h>
#include <dpp/nlohmann/json.hpp>
 
using json = nlohmann::json;

int main()
{
	json configdocument;
	std::ifstream configfile("../config.json");
	configfile >> configdocument;
	dpp::cluster bot(configdocument["token"]);

	bot.on_ready([&bot](const dpp::ready_t & event) {
	});

	bot.on_log([&](const dpp::log_t& loginfo) {
		if (loginfo.severity >= dpp::ll_trace) {
			std::cout << dpp::utility::loglevel(loginfo.severity) << " " << loginfo.message << "\n";
		}
	});

	bot.start(false);

	return 0;
}