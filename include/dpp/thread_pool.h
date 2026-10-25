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

/**
 * @brief A work unit is a lambda executed in the thread pool
 */
using work_unit = std::function<void()>;

/**
 * @brief A task within a thread pool. A simple lambda that accepts no parameters and returns void.
 */
struct DPP_EXPORT thread_pool_task {
	/**
	 * @brief Task priority, lower value is higher priority
	 */
	int priority;
	/**
	 * @brief Work unit to execute as the task
	 */
	work_unit function;
};

/**
 * @brief Compares two thread pool tasks by priority
 */
struct DPP_EXPORT thread_pool_task_comparator {
	/**
	 * @brief Compare two tasks
	 * @param a first task
	 * @param b second task
	 * @return true if a > b
	 */
	bool operator()(const thread_pool_task &a, const thread_pool_task &b) const {
		return a.priority > b.priority;
	};
};

/**
 * @brief A thread pool contains 1 or more worker threads which accept thread_pool_task lambadas
 * into a queue, which is processed in-order by whichever thread is free.
 */
struct DPP_EXPORT thread_pool {

	/**
	 * @brief Threads that comprise the thread pool
	 */
	std::vector<std::thread> threads;

	/**
	 * @brief Priority queue of tasks to be executed
	 */
	std::priority_queue<thread_pool_task, std::vector<thread_pool_task>, thread_pool_task_comparator> tasks;

	/**
	 * @brief Mutex for accessing the priority queue
	 */
	std::mutex queue_mutex;

	/**
	 * @brief Condition variable to notify for new tasks to run
	 */
	std::condition_variable cv;

	/**
	 * @brief True if the thread pool is due to stop
	 */
	bool stop{false};

	/**
	 * @brief Create a new priority thread pool
	 * @param creator creating cluster (for logging)
	 * @param num_threads number of threads in the pool
	 */
	explicit thread_pool(class cluster* creator, size_t num_threads = std::thread::hardware_concurrency());

	/**
	 * @brief Destroy the thread pool
	 */
	~thread_pool();

	/**
	 * @brief Enqueue a new task to the thread pool
	 * @param task task to enqueue
	 */
	void enqueue(thread_pool_task task);
};

}