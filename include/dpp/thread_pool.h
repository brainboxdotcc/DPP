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

#pragma once
#include <thread>
#include <queue>
#include <vector>
#include <condition_variable>
#include <mutex>
#include <functional>

namespace dpp {

using work_unit = std::function<void()>;

/**
 * A task within a thread pool. A simple lambda that accepts no parameters and returns void.
 */
struct DPP_EXPORT thread_pool_task {
	int priority;
	work_unit function;
};

struct DPP_EXPORT thread_pool_task_comparator {
	bool operator()(const thread_pool_task &a, const thread_pool_task &b) const {
		return a.priority > b.priority;
	};
};

/**
 * @brief A thread pool contains 1 or more worker threads which accept thread_pool_task lambadas
 * into a queue, which is processed in-order by whichever thread is free.
 */
struct DPP_EXPORT thread_pool {
	std::vector<std::thread> threads;
	std::priority_queue<thread_pool_task, std::vector<thread_pool_task>, thread_pool_task_comparator> tasks;
	std::mutex queue_mutex;
	std::condition_variable cv;
	bool stop{false};

	/**
	 * @brief Create a new priority thread pool
	 * @param creator creating cluster (for logging)
	 * @param num_threads number of threads in the pool
	 */
	explicit thread_pool(class cluster* creator, size_t num_threads = std::thread::hardware_concurrency());
	~thread_pool();
	void enqueue(thread_pool_task task);
};

}