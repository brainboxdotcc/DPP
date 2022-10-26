\page making_a_http_request Making arbitrary HTTP requests using D++

If you wish to make arbitrary HTTP(S) requests to websites and APIs, e.g. to update statistics on bot lists, you can use code similar to the code below. You may pass any arbitrary POST data:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main() {
	dpp::cluster bot("TOKEN GOES HERE");

	bot.on_log(dpp::utility::cout_logger());

	bot.on_ready([&bot](const dpp::ready_t& event) {
		// Arbitrary post data as a string
		std::string mypostdata = "{\"value\": 42}";
		// Make a HTTP POST request. HTTP and HTTPS are supported here.
		bot.request(
			"http://www.somebotlist.com/api/servers", dpp::m_post, [](const dpp::http_request_completion_t & cc) {
				// This callback is called when the HTTP request completes. See documentation of
				// dpp::http_request_completion_t for information on the fields in the parameter.
				std::cout << "I got reply: " << cc.body << " with HTTP status code: " << cc.status << "\n";
			},
			mypostdata,
			"application/json",
			{
				{"Authorization", "Bearer tokengoeshere"}
			}
		);
	});

	bot.start(dpp::st_wait);
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
