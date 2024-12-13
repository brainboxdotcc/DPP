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
#include <dpp/export.h>
#include <dpp/queues.h>
#include <dpp/cluster.h>
#include <dpp/httpsclient.h>
#ifdef _WIN32
	#include <io.h>
#endif

namespace dpp {

/**
 * @brief List of possible request verbs.
 *
 * This MUST MATCH the size of the dpp::http_method enum!
 */
constexpr std::array request_verb {
	"GET",
	"POST",
	"PUT",
	"PATCH",
	"DELETE"
};

namespace
{

/**
 * @brief Comparator for sorting a request container
 */
struct compare_request {
	/**
	 * @brief Less_than comparator for sorting
	 * @param lhs Left-hand side
	 * @param rhs Right-hand side
	 * @return Whether lhs comes before rhs in strict ordering
	 */
	bool operator()(const std::unique_ptr<http_request>& lhs, const std::unique_ptr<http_request>& rhs) const noexcept {
		return std::less{}(lhs->endpoint, rhs->endpoint);
	};

	/**
	 * @brief Less_than comparator for sorting
	 * @param lhs Left-hand side
	 * @param rhs Right-hand side
	 * @return Whether lhs comes before rhs in strict ordering
	 */
	bool operator()(const std::unique_ptr<http_request>& lhs, std::string_view rhs) const noexcept {
		return std::less{}(lhs->endpoint, rhs);
	};

	/**
	 * @brief Less_than comparator for sorting
	 * @param lhs Left-hand side
	 * @param rhs Right-hand side
	 * @return Whether lhs comes before rhs in strict ordering
	 */
	bool operator()(std::string_view lhs, const std::unique_ptr<http_request>& rhs) const noexcept {
		return std::less{}(lhs, rhs->endpoint);
	};
};

}

http_request::http_request(const std::string &_endpoint, const std::string &_parameters, http_completion_event completion, const std::string &_postdata, http_method _method, const std::string &audit_reason, const std::string &filename, const std::string &filecontent, const std::string &filemimetype, const std::string &http_protocol)
 : complete_handler(completion), completed(false), non_discord(false), endpoint(_endpoint), parameters(_parameters), postdata(_postdata),  method(_method), reason(audit_reason), mimetype("application/json"), waiting(false), protocol(http_protocol)
{
	if (!filename.empty()) {
		file_name.push_back(filename);
	}
	if (!filecontent.empty()) {
		file_content.push_back(filecontent);
	}
	if (!filemimetype.empty()) {
		file_mimetypes.push_back(filemimetype);
	}
}

http_request::http_request(const std::string &_endpoint, const std::string &_parameters, http_completion_event completion, const std::string &_postdata, http_method method, const std::string &audit_reason, const std::vector<std::string> &filename, const std::vector<std::string> &filecontent, const std::vector<std::string> &filemimetypes, const std::string &http_protocol)
 : complete_handler(completion), completed(false), non_discord(false), endpoint(_endpoint), parameters(_parameters), postdata(_postdata),  method(method), reason(audit_reason), file_name(filename), file_content(filecontent), file_mimetypes(filemimetypes), mimetype("application/json"), waiting(false), protocol(http_protocol)
{
}


http_request::http_request(const std::string &_url, http_completion_event completion, http_method _method, const std::string &_postdata, const std::string &_mimetype, const std::multimap<std::string, std::string> &_headers, const std::string &http_protocol)
 : complete_handler(completion), completed(false), non_discord(true), endpoint(_url), postdata(_postdata), method(_method), mimetype(_mimetype), req_headers(_headers), waiting(false), protocol(http_protocol)
{
}

http_request::~http_request()  = default;

void http_request::complete(const http_request_completion_t &c) {
	if (complete_handler) {
		complete_handler(c);
	}
}

/* Fill a http_request_completion_t from a HTTP result */
void populate_result(const std::string &url, cluster* owner, http_request_completion_t& rv, const https_client &res) {
	rv.status = res.get_status();
	rv.body = res.get_content();
	for (auto &v : res.get_headers()) {
		rv.headers.emplace(v.first, v.second);
	}

	/* This will be ignored for non-discord requests without rate limit headers */

	rv.ratelimit_limit = from_string<uint64_t>(res.get_header("x-ratelimit-limit"));
	rv.ratelimit_remaining = from_string<uint64_t>(res.get_header("x-ratelimit-remaining"));
	rv.ratelimit_reset_after = from_string<uint64_t>(res.get_header("x-ratelimit-reset-after"));
	rv.ratelimit_bucket = res.get_header("x-ratelimit-bucket");
	rv.ratelimit_global = (res.get_header("x-ratelimit-global") == "true");
	owner->rest_ping = rv.latency;
	if (res.get_header("x-ratelimit-retry-after") != "") {
		rv.ratelimit_retry_after = from_string<uint64_t>(res.get_header("x-ratelimit-retry-after"));
	}
	uint64_t rl_timer = rv.ratelimit_retry_after ? rv.ratelimit_retry_after : rv.ratelimit_reset_after;
	if (rv.status == 429) {
		owner->log(ll_warning, "Rate limited on endpoint " + url + ", reset after " + std::to_string(rl_timer) + "s!");
	}
	if (url != "/api/v" DISCORD_API_VERSION "/gateway/bot") {	// Squelch this particular api endpoint or it generates a warning the minute we boot a cluster
		if (rv.ratelimit_global) {
			owner->log(ll_warning, "At global rate limit on endpoint " + url + ", reset after " + std::to_string(rl_timer) + "s!");
		} else if (rv.ratelimit_remaining == 0 && rl_timer > 0) {
			owner->log(ll_debug, "Waiting for endpoint " + url + " rate limit, next request in " + std::to_string(rl_timer) + "s");
		}
	}
}

/* Returns true if the request has been made */
bool http_request::is_completed()
{
	return completed;
}

https_client* http_request::get_client() const
{
	return cli.get();
}

/* Execute a HTTP request */
http_request_completion_t http_request::run(request_concurrency_queue* processor, cluster* owner) {

	http_request_completion_t rv;
	double start = dpp::utility::time_f();
	std::string _host = DISCORD_HOST;
	std::string _url = endpoint;

	if (non_discord) {
		std::size_t s_start = endpoint.find("://", 0);
		if (s_start != std::string::npos) {
			s_start += 3; /* "://" */
			/**
			 * NOTE: "#" is in this list, really # is client side only.
			 * This won't stop some moron from using it as part of an
			 * API endpoint...
			 */
			std::size_t s_end = endpoint.find_first_of("/?#", s_start + 1);
			if (s_end != std::string::npos) {
				_host = endpoint.substr(0, s_end);
				_url = endpoint.substr(s_end);
			} else {
				_host = endpoint;
				_url.clear();
			}
		} else {
			owner->log(ll_error, "Request to '" + endpoint + "' missing protocol scheme. This is not supported. Please specify http or https.");
		}
	}

	rv.ratelimit_limit = rv.ratelimit_remaining = rv.ratelimit_reset_after = rv.ratelimit_retry_after = 0;
	rv.status = 0;
	rv.latency = 0;
	rv.ratelimit_global = false;

	dpp::http_headers headers;
	if (non_discord) {
		/* Requests outside of Discord have their own headers an NEVER EVER send a bot token! */
		for (auto& r : req_headers) {
			headers.emplace(r.first, r.second);
		};
	} else {
		/* Always attach token and correct user agent when sending REST to Discord */
		headers.emplace("Authorization", "Bot " + owner->token);
		headers.emplace("User-Agent", http_version);
		if (!reason.empty()) {
			headers.emplace("X-Audit-Log-Reason", reason);
		}
		if (!empty(parameters)) {
			_url = endpoint + "/" +parameters;
		}
	}

	multipart_content multipart;
	if (non_discord) {
		multipart = { postdata, mimetype };
	} else {
		multipart = https_client::build_multipart(postdata, file_name, file_content, file_mimetypes);
	}
	if (!multipart.mimetype.empty()) {
		headers.emplace("Content-Type", multipart.mimetype);
	}
	http_connect_info hci = https_client::get_host_info(_host);
	try {
		cli = std::make_unique<https_client>(
			owner,
			hci.hostname,
			hci.port,
			_url,
			request_verb[method],
			multipart.body,
			headers,
			!hci.is_ssl,
			owner->request_timeout,
			protocol,
			[processor, rv, hci, this, owner, start, _url](https_client* client) {
				http_request_completion_t result{rv};
				result.latency = dpp::utility::time_f() - start;
				if (client->timed_out) {
					result.error = h_connection;
					owner->log(ll_error, "HTTP(S) error on " + hci.scheme + " connection to " + request_verb[method] + " "  + hci.hostname + ":" + std::to_string(hci.port) + _url + ": Timed out while waiting for the response");
				} else if (client->get_status() < 100) {
					result.error = h_connection;
					owner->log(ll_error, "HTTP(S) error on " + hci.scheme + " connection to " + request_verb[method] + " "  + hci.hostname + ":" + std::to_string(hci.port) + _url + ": Malformed HTTP response");
				}
				populate_result(_url, owner, result, *client);
				/* Set completion flag */

				bucket_t newbucket;
				newbucket.limit = result.ratelimit_limit;
				newbucket.remaining = result.ratelimit_remaining;
				newbucket.reset_after = result.ratelimit_reset_after;
				newbucket.retry_after = result.ratelimit_retry_after;
				newbucket.timestamp = time(nullptr);
				processor->requests->globally_ratelimited = rv.ratelimit_global;
				if (processor->requests->globally_ratelimited) {
					/* We are globally rate limited - user up to shenanigans */
					processor->requests->globally_limited_until = (newbucket.retry_after ? newbucket.retry_after : newbucket.reset_after) + newbucket.timestamp;
				}
				processor->buckets[this->endpoint] = newbucket;

				/* Transfer it to completed requests */
				owner->queue_work(0, [owner, this, result, hci, _url]() {
					try {
						complete(result);
					}
					catch (const std::exception& e) {
						owner->log(ll_error, "Uncaught exception thrown in HTTPS callback for " + std::string(request_verb[method]) + " "  + hci.hostname + ":" + std::to_string(hci.port) + _url + ": " + std::string(e.what()));
					}
					catch (...) {
						owner->log(ll_error, "Uncaught exception thrown in HTTPS callback for " + std::string(request_verb[method]) + " "  + hci.hostname + ":" + std::to_string(hci.port) + _url + ": <non exception value>");
					}
					completed = true;
				});
			}
		);
	}
	catch (const std::exception& e) {
		owner->log(ll_error, "HTTP(S) error on " + hci.scheme + " connection to " + hci.hostname + ":" + std::to_string(hci.port) + ": " + std::string(e.what()));
		rv.error = h_connection;
	}
	return rv;
}

request_queue::request_queue(class cluster* owner, uint32_t request_concurrency) : creator(owner), terminating(false), globally_ratelimited(false), globally_limited_until(0), in_queue_pool_size(request_concurrency)
{
	/* Create request_concurrency timer instances */
	for (uint32_t in_alloc = 0; in_alloc < in_queue_pool_size; ++in_alloc) {
		requests_in.push_back(std::make_unique<request_concurrency_queue>(owner, this, in_alloc));
	}
}

uint32_t request_queue::get_request_queue_count() const
{
	return in_queue_pool_size;
}

request_concurrency_queue::request_concurrency_queue(class cluster* owner, class request_queue* req_q, uint32_t index) : in_index(index), terminating(false), requests(req_q), creator(owner)
{
	in_timer = creator->start_timer([this](auto timer_handle) {
		tick_and_deliver_requests(in_index);
		/* Clear pending removals in the removals queue */
		if (time(nullptr) % 90 == 0) {
			std::scoped_lock lock1{in_mutex};
			for (auto it = removals.cbegin(); it != removals.cend();) {
				if ((*it)->is_completed()) {
					it = removals.erase(it);
				} else {
					++it;
				}
			}
		}
	}, 1);
}

request_concurrency_queue::~request_concurrency_queue()
{
	terminate();
	creator->stop_timer(in_timer);
}

void request_concurrency_queue::terminate()
{
	terminating.store(true, std::memory_order_relaxed);
}

request_queue::~request_queue()
{
	terminating.store(true, std::memory_order_relaxed);
	for (auto& in_thr : requests_in) {
		/* Note: We don't need to set the atomic to make timers quit, this is purely
		 * to prevent additional requests going into the queue while it is being destructed
		 * from other threads,
		 */
		in_thr->terminate();
	}
}

void request_concurrency_queue::tick_and_deliver_requests(uint32_t index)
{
	if (terminating) {
		return;
	}

	if (!requests->globally_ratelimited) {

		std::vector<http_request*> requests_view;
		{
			/* Gather all the requests first within a mutex */
			std::shared_lock lock(in_mutex);
			if (requests_in.empty()) {
				/* Nothing to copy, check again when we call the timer in a second */
				return;
			}
			requests_view.reserve(requests_in.size());
			std::transform(requests_in.begin(), requests_in.end(), std::back_inserter(requests_view), [](const std::unique_ptr<http_request> &r) {
				return r.get();
			});
		}

		for (auto& request_view : requests_view) {
			const std::string &key = request_view->endpoint;
			http_request_completion_t rv;
			auto currbucket = buckets.find(key);

			if (currbucket != buckets.end()) {
				/* There's a bucket for this request. Check its status. If the bucket says to wait,
				 * skip all requests until the timer value indicates the rate limit won't be hit
				 */
				if (currbucket->second.remaining < 1) {
					uint64_t wait = (currbucket->second.retry_after ? currbucket->second.retry_after : currbucket->second.reset_after);
					if ((uint64_t)time(nullptr) > currbucket->second.timestamp + wait) {
						/* Time has passed, we can process this bucket again. send its request. */
						request_view->run(this, creator);
					} else {
						if (!request_view->waiting) {
							request_view->waiting = true;
						}
						/* Time not up yet, wait more */
						break;
					}
				} else {
					/* We aren't at the limit, so we can just run the request */
					request_view->run(this, creator);
				}
			} else {
				/* No bucket for this endpoint yet. Just send it, and make one from its reply */
				request_view->run(this, creator);
			}

			/* Remove from inbound requests */
			std::unique_ptr<http_request> rq;
			{
				/* Find the owned pointer in requests_in */
				std::scoped_lock lock1{in_mutex};

				const std::string &key = request_view->endpoint;
				auto [begin, end] = std::equal_range(requests_in.begin(), requests_in.end(), key, compare_request{});
				for (auto it = begin; it != end; ++it) {
					if (it->get() == request_view) {
						/* Grab and remove */
						rq = std::move(*it);
						removals.push_back(std::move(rq));
						requests_in.erase(it);
						break;
					}
				}
			}
		}

	} else {
		/* If we are globally rate limited, do nothing until we are not */
		if (time(nullptr) > requests->globally_limited_until) {
			requests->globally_limited_until = 0;
			requests->globally_ratelimited = false;
		}
	}
}

/* Post a http_request into the queue */
void request_concurrency_queue::post_request(std::unique_ptr<http_request> req)
{
	{
		std::scoped_lock lock(in_mutex);

		auto where = std::lower_bound(requests_in.begin(), requests_in.end(), req->endpoint, compare_request{});
		requests_in.emplace(where, std::move(req));
	}
	/* Immediately trigger requests in this queue */
	tick_and_deliver_requests(in_index);
}

/* @brief Simple hash function for hashing urls into request pool values,
 * ensuring that the same url always ends up in the same queue,
 * which means that it will be part of the same ratelimit bucket.
 * I did consider std::hash for this, but std::hash returned even
 * numbers for absolutely every string i passed it on g++ 10.0,
 * so this was a no-no. There are also much bigger more complex
 * hash functions that claim to be really fast, but this is
 * readable and small and fits the requirement exactly.
 *
 * @param s String to hash
 * @return Hash value
 */
inline uint32_t hash(const char *s)
{
	uint32_t hashval;
	for (hashval = 17; *s != 0; s++) {
		hashval = *s + 31 * hashval;
	}
	return hashval;
}

/* Post a http_request into a request queue */
request_queue& request_queue::post_request(std::unique_ptr<http_request> req) {
	if (!terminating) {
		requests_in[hash(req->endpoint.c_str()) % in_queue_pool_size]->post_request(std::move(req));
	}
	return *this;
}

bool request_queue::is_globally_ratelimited() const {
	return this->globally_ratelimited;
}

size_t request_queue::get_active_request_count() const {
	size_t total{};
	for (auto& pool : requests_in) {
		std::scoped_lock lock(pool->in_mutex);
		total += pool->requests_in.size();
	}
	return total;
}

}
