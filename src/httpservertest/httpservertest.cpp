/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <dpp/dpp.h>
#include <iostream>

/**
 * @brief Generate a simple html response
 * @param request request to respond to
 */
void respond(dpp::http_server_request& request) {
	request.set_status(200).set_response_header("Content-Type", "text/html").set_response_body("<h1>It lives!</h1>");
}

int main() {
	using namespace std::chrono_literals;
	char* t = getenv("DPP_UNIT_TEST_TOKEN");
	if (t) {
		dpp::cluster bot(t, 0, dpp::NO_SHARDS, 1, 1, false, dpp::cache_policy::cpol_none);
		dpp::http_server *server1{nullptr}, *server2{nullptr};

		bot.on_log([&](const dpp::log_t& log) {
			std::cout << "[" << dpp::utility::current_date_time() << "] " << dpp::utility::loglevel(log.severity) << ": " << log.message << std::endl;
		});
		bot.on_ready([&](const dpp::ready_t& ready) {

			/* A plaintext HTTP server on port 3011 */
			server1 = new dpp::http_server(&bot, "0.0.0.0", 3011, [&bot](dpp::http_server_request* request) {
				respond(*request);
			});

			/* An SSL enabled HTTPS server on port 3042 */
			server2 = new dpp::http_server(&bot, "0.0.0.0", 3042, [&bot](dpp::http_server_request* request) {
				respond(*request);
			}, "../../testdata/localhost.key", "../../testdata/localhost.pem");

			/* A discord interactions endpoint on port 3010 */
			bot.enable_webhook_server(getenv("DPP_PUBLIC_KEY"), "0.0.0.0", 3010);
		});
		bot.on_slashcommand([&](const dpp::slashcommand_t& event) {
			event.reply("hello");
		});

		bot.start(dpp::st_wait);
	}
}
