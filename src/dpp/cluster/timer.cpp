/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
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
#include <dpp/timer.h>
#include <dpp/cluster.h>
#include <dpp/json.h>
#include <atomic>

namespace dpp {

std::atomic<timer> next_handle = 1;

timer cluster::start_timer(timer_callback_t on_tick, uint64_t frequency, timer_callback_t on_stop) {
	timer_t new_timer;

	new_timer.handle = next_handle++;
	new_timer.next_tick = time(nullptr) + frequency;
	new_timer.on_tick = std::move(on_tick);
	new_timer.on_stop = std::move(on_stop);
	new_timer.frequency = frequency;

	std::lock_guard<std::mutex> l(timer_guard);
	next_timer.emplace(new_timer);

	return new_timer.handle;
}

bool cluster::stop_timer(timer t) {
	/*
	 * Because iterating a priority queue is at best O(log n) we don't actually walk the queue
	 * looking for the timer to remove. Instead, we just insert the timer handle into a std::set
	 * to inform the tick_timers() function later if it sees a handle in this set, it is to
	 * have its on_stop() called and it is not to be rescheduled.
	 */
	std::lock_guard<std::mutex> l(timer_guard);
	deleted_timers.emplace(t);
	return true;
}

void cluster::tick_timers() {
	time_t now = time(nullptr);

	if (next_timer.empty()) {
		return;
	}
	do {
		timer_t cur_timer;
		{
			std::lock_guard<std::mutex> l(timer_guard);
			if (next_timer.top().next_tick > now) {
				/* Nothing to do */
				break;
			}
			cur_timer = std::move(next_timer.top());
			next_timer.pop();
		}
		timers_deleted_t::iterator deleted_iter{};
		bool deleted{};
		{
			std::lock_guard<std::mutex> l(timer_guard);
			deleted_iter = deleted_timers.find(cur_timer.handle);
			deleted = deleted_iter != deleted_timers.end();
		}

		if (!deleted) {
			cur_timer.on_tick(cur_timer.handle);
			cur_timer.next_tick += cur_timer.frequency;
			{
				std::lock_guard<std::mutex> l(timer_guard);
				next_timer.emplace(std::move(cur_timer));
			}
		} else {
			/* Deleted timers are not reinserted into the priority queue and their on_stop is called */
			if (cur_timer.on_stop) {
				cur_timer.on_stop(cur_timer.handle);
			}
			{
				std::lock_guard<std::mutex> l(timer_guard);
				deleted_timers.erase(deleted_iter);
			}
		}

	} while (true);
}

#ifndef DPP_NO_CORO
async<timer> cluster::co_sleep(uint64_t seconds) {
	return async<timer>{[this, seconds] (auto &&cb) mutable {
		start_timer([this, cb] (dpp::timer handle) {
			cb(handle);
			stop_timer(handle);
		}, seconds);
	}};
}
#endif

oneshot_timer::oneshot_timer(class cluster* cl, uint64_t duration, timer_callback_t callback) : owner(cl) {
	/* Create timer */
	th = cl->start_timer([callback, this](dpp::timer timer_handle) {
		callback(this->get_handle());
		this->owner->stop_timer(this->th);
	}, duration);
}

timer oneshot_timer::get_handle() {
	return this->th;
}

void oneshot_timer::cancel() {
	owner->stop_timer(this->th);
}

oneshot_timer::~oneshot_timer() {
	cancel();
}

}
