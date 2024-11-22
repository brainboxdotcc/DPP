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

#include <dpp/utility.h>
#include <dpp/thread_pool.h>
#include <shared_mutex>
#include <dpp/cluster.h>

namespace dpp {

thread_pool::thread_pool(cluster* creator, size_t num_threads) {
	for (size_t i = 0; i < num_threads; ++i) {
		threads.emplace_back([this, i, creator]() {
			dpp::utility::set_thread_name("pool/exec/" + std::to_string(i));
			while (true) {
				thread_pool_task task;
				{
					std::unique_lock<std::mutex> lock(queue_mutex);

					cv.wait(lock, [this] {
						return !tasks.empty() || stop;
					});

					if (stop && tasks.empty()) {
						return;
					}

					task = std::move(tasks.top());
					tasks.pop();
				}

				try {
					task.function();
				}
				catch (const std::exception &e) {
					creator->log(ll_warning, "Uncaught exception in thread pool: " + std::string(e.what()));
				}
				catch (...) {
					creator->log(ll_warning, "Uncaught exception in thread pool, but not derived from std::exception!");
				}
			}
		});
	}
}

thread_pool::~thread_pool() {
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		stop = true;
	}

	cv.notify_all();
	for (auto &thread: threads) {
		thread.join();
	}
}

void thread_pool::enqueue(thread_pool_task task) {
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		tasks.emplace(std::move(task));
	}
	cv.notify_one();
}

}