#include <dpp/dpp.h>
#include <fstream>
#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

using json = nlohmann::json;


int main(int argc, char const *argv[])
{
	json configdocument;
	std::ifstream configfile("../config.json");
	configfile >> configdocument;

	/* An example of how to set up an spdlog logger like the one in aegis */
	std::shared_ptr<spdlog::logger> log;
	spdlog::init_thread_pool(8192, 2);
	std::vector<spdlog::sink_ptr> sinks;
	auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
	auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("test.txt", 1024 * 1024 * 5, 10);
	sinks.push_back(stdout_sink);
	sinks.push_back(rotating);
	log = std::make_shared<spdlog::async_logger>("test", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
	spdlog::register_logger(log);
	log->set_pattern("%^%Y-%m-%d %H:%M:%S.%e [%L] [th#%t]%$ : %v");
	log->set_level(spdlog::level::level_enum::debug);

	log->info("Starting test bot");
	DiscordClient client(0, 1, configdocument["token"], log.get());
	client.Run();
	client.close();
	return 0;
}

