\page spdlog Integrating with spdlog

If you want to make your bot use spdlog, like aegis does, you can attach it to the on_log event. You can do this as follows:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <iomanip>
#include <dpp/dpp.h>
#include <fmt/format.h>

int main(int argc, char const *argv[])
{
	dpp::cluster bot("token");

	const std::string log_name = "mybot.log";

	/* Set up spdlog logger */
	std::shared_ptr<spdlog::logger> log;
	spdlog::init_thread_pool(8192, 2);
	std::vector<spdlog::sink_ptr> sinks;
	auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
	auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_name, 1024 * 1024 * 5, 10);
	sinks.push_back(stdout_sink);
	sinks.push_back(rotating);
	log = std::make_shared<spdlog::async_logger>("logs", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
	spdlog::register_logger(log);
	log->set_pattern("%^%Y-%m-%d %H:%M:%S.%e [%L] [th#%t]%$ : %v");
	log->set_level(spdlog::level::level_enum::debug);

	/* Integrate spdlog logger to D++ log events */
	bot.on_log([&bot, &log](const dpp::log_t & event) {
		switch (event.severity) {
			case dpp::ll_trace:
				log->trace("{}", event.message);
			break;
			case dpp::ll_debug:
				log->debug("{}", event.message);
			break;
			case dpp::ll_info:
				log->info("{}", event.message);
			break;
			case dpp::ll_warning:
				log->warn("{}", event.message);
			break;
			case dpp::ll_error:
				log->error("{}", event.message);
			break;
			case dpp::ll_critical:
			default:
				log->critical("{}", event.message);
			break;
		}
	});

	/* Add the rest of your events */

	bot.start(dpp::st_wait);
	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
