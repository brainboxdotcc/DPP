#undef DPP_BUILD
#include <dpp/dpp.h>
#include <dpp/fmt/format.h>
#include <dpp/nlohmann/json.hpp>
 
using json = nlohmann::json;

int main()
{
	json configdocument;
	std::ifstream configfile("../config.json");
	configfile >> configdocument;
	dpp::cluster bot(configdocument["token"]);

	bot.on_ready([&bot](const dpp::ready_t & event) {
		bot.request("https://brainbox.cc/robots.txt", dpp::m_get, [&bot](const dpp::http_request_completion_t& rv) {
			std::cout << rv.status << rv.body << "\n";
		});
	});

	bot.start(false);

	return 0;
}