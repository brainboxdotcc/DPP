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

int64_t proc_self_value(const std::string& find_token) {
	int64_t ret = 0;
	std::ifstream self_status("/proc/self/status");
	while (self_status) {
		std::string token;
		self_status >> token;
		if (token == find_token) {
			self_status >> ret;
			break;
		}
	}
	self_status.close();
	return ret;
}

int64_t rss() {
	return proc_self_value("VmRSS:") * 1024;
}

#ifdef DPP_CORO
dpp::task<void> test(dpp::cluster& cluster) {
	[[maybe_unused]] int test[102400]{};
	test[60] = 5;
	co_return;
}
#endif

int main() {
#ifdef DPP_CORO
	char* t = getenv("DPP_UNIT_TEST_TOKEN");
	if (t) {
		dpp::cluster coro_cluster(t, dpp::i_guilds, 1, 0, 1, true, dpp::cache_policy::cpol_none);
		coro_cluster.set_websocket_protocol(dpp::ws_etf);
		coro_cluster.on_log(dpp::utility::cout_logger());
		coro_cluster.on_ready([&coro_cluster](auto) -> dpp::task<void> {
			coro_cluster.start_timer([&coro_cluster](dpp::timer) -> dpp::task<void> {
				for (int i = 0; i < 1000; ++i) {
					co_await test(coro_cluster);
				}
				coro_cluster.log(dpp::ll_info, "coro timer ticked. RSS=" + std::to_string(rss()));	
				co_return;
			}, 1);
			co_return;
		});
		coro_cluster.start(dpp::st_wait);
	}
#endif
}
