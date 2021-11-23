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
#ifdef _WIN32
/* Central point for forcing inclusion of winsock library for all socket code */
#include <io.h>
#pragma comment(lib,"ws2_32")
#endif
#include <dpp/queues.h>
#include <dpp/cluster.h>
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <dpp/httplib.h>
#include <dpp/fmt/format.h>
#include <dpp/stringops.h>
#include <dpp/version.h>

namespace dpp {

static std::string http_version = "DiscordBot (https://github.com/brainboxdotcc/DPP, " + std::to_string(DPP_VERSION_MAJOR) + "." + std::to_string(DPP_VERSION_MINOR) + "." + std::to_string(DPP_VERSION_PATCH) + ")";
static const char* DISCORD_HOST = "https://discord.com";

http_request::http_request(const std::string &_endpoint, const std::string &_parameters, http_completion_event completion, const std::string &_postdata, http_method _method, const std::string &audit_reason, const std::string &filename, const std::string &filecontent)
 : complete_handler(completion), completed(false), non_discord(false), endpoint(_endpoint), parameters(_parameters), postdata(_postdata),  method(_method), reason(audit_reason), file_name(filename), file_content(filecontent), mimetype("application/json")
{
}

http_request::http_request(const std::string &_url, http_completion_event completion, http_method _method, const std::string &_postdata, const std::string &_mimetype, const std::multimap<std::string, std::string> &_headers)
 : complete_handler(completion), completed(false), non_discord(true), endpoint(_url), postdata(_postdata), method(_method), mimetype(_mimetype), req_headers(_headers)
{
}

http_request::~http_request() = default;

void http_request::complete(const http_request_completion_t &c) {
	/* Call completion handler only if the request has been completed */
	if (is_completed() && complete_handler)
		complete_handler(c);
}

/* Fill a http_request_completion_t from a HTTP result */
void populate_result(const std::string &url, cluster* owner, http_request_completion_t& rv, const httplib::Result &res) {
	rv.status = res->status;
	rv.body = res->body;
	for (auto &v : res->headers) {
		rv.headers[v.first] = v.second;
	}

	/* This will be ignored for non-discord requests without rate limit headers */

	rv.ratelimit_limit = from_string<uint64_t>(res->get_header_value("X-RateLimit-Limit"));
	rv.ratelimit_remaining = from_string<uint64_t>(res->get_header_value("X-RateLimit-Remaining"));
	rv.ratelimit_reset_after = from_string<uint64_t>(res->get_header_value("X-RateLimit-Reset-After"));
	rv.ratelimit_bucket = res->get_header_value("X-RateLimit-Bucket");
	rv.ratelimit_global = (res->get_header_value("X-RateLimit-Global") == "true");
	owner->rest_ping = rv.latency;
	if (res->get_header_value("X-RateLimit-Retry-After") != "") {
		rv.ratelimit_retry_after = from_string<uint64_t>(res->get_header_value("X-RateLimit-Retry-After"));
	}
	if (rv.status == 429) {
		owner->log(ll_warning, fmt::format("Rate limited on endpoint {}, reset after {}s!", url, rv.ratelimit_retry_after ? rv.ratelimit_retry_after : rv.ratelimit_reset_after));
	}
	if (url != "/api/v" DISCORD_API_VERSION "/gateway/bot") {	// Squelch this particular api endpoint or it generates a warning the minute we boot a cluster
		if (rv.ratelimit_global) {
			owner->log(ll_warning, fmt::format("At global rate limit on endpoint {}, reset after {}s", url, rv.ratelimit_retry_after ? rv.ratelimit_retry_after : rv.ratelimit_reset_after));
		} else if (rv.ratelimit_remaining == 1) {
			owner->log(ll_warning, fmt::format("Near endpoint {} rate limit, reset after {}s", url, rv.ratelimit_retry_after ? rv.ratelimit_retry_after : rv.ratelimit_reset_after));
		}
	}
}

/* Returns true if the request has been made */
bool http_request::is_completed()
{
	return completed;
}

/* Execute a HTTP request */
http_request_completion_t http_request::run(cluster* owner) {

	http_request_completion_t rv;
	double start = dpp::utility::time_f();
	std::string _host = DISCORD_HOST;
	std::string _url = endpoint;

	if (non_discord) {
		std::size_t s_start = endpoint.find("://", 0);
		if (s_start != std::string::npos) {
			s_start += 3; /* "://" */
			std::size_t s_end = endpoint.find("/", s_start + 1);
			_host = endpoint.substr(0, s_end);
			_url = endpoint.substr(s_end);	
		}
	}

	httplib::Client cli(_host.c_str());
	/* This is for a reason :( - Some systems have really out of date cert stores */
	cli.enable_server_certificate_verification(false);
	cli.set_follow_location(true);

	rv.ratelimit_limit = rv.ratelimit_remaining = rv.ratelimit_reset_after = rv.ratelimit_retry_after = 0;
	rv.status = 0;
	rv.latency = 0;
	rv.ratelimit_global = false;

	if (non_discord) {
		/* Requests outside of Discord have their own headers an NEVER EVER send a bot token! */
		httplib::Headers headers;
		for (auto& r : req_headers) {
			headers.emplace(r.first, r.second);
		};
		cli.set_default_headers(headers);
	} else {
		/* Always attach token and correct user agent when sending REST to Discord */
		httplib::Headers headers = {
			{"Authorization", std::string("Bot ") + owner->token},
			{"User-Agent", http_version}
		};
		if (!reason.empty()) {
			headers.emplace("X-Audit-Log-Reason", reason);
		}
		cli.set_default_headers(headers);
		if (!empty(parameters)) {
			_url = endpoint + "/" +parameters;
		}
	}

	/* Because of the design of cpp-httplib we can't create a httplib::Result once and make this code
	 * shorter. We have to use "auto res = ...". This is because httplib::Result has no default constructor
	 * and needs to be passed a result and some other blackboxed rammel.
	 */
	switch (method) {
		case m_get: {
			if (auto res = cli.Get(_url.c_str())) {
				rv.latency = dpp::utility::time_f() - start;
				populate_result(_url, owner, rv, res);
			} else {
				rv.error = (http_error)res.error();
			}
		}
		break;
		case m_post: {
			/* POST supports post data body */
			if (!file_name.empty() && !file_content.empty()) {
				if (non_discord) {
					/* Outside Discord just post a standard non-multipart POST body */
					if (auto res = cli.Post(_url.c_str(), postdata, mimetype.c_str())) {
						rv.latency = dpp::utility::time_f() - start;
						populate_result(_url, owner, rv, res);
					} else {
						rv.error = (http_error)res.error();
					}
				} else {
					/* Special multipart mime body for Discord */
					httplib::MultipartFormDataItems items = {
						{ "payload_json", postdata, "", mimetype.c_str() },
						{ "file", file_content, file_name, "application/octet-stream" }
					};
					if (auto res = cli.Post(_url.c_str(), items)) {
						rv.latency = dpp::utility::time_f() - start;
						populate_result(_url, owner, rv, res);
					} else {
						rv.error = (http_error)res.error();
					}
				}
			} else {
				/* Without a filename attached, use the same for both */
				if (auto res = cli.Post(_url.c_str(), postdata, mimetype.c_str())) {
					rv.latency = dpp::utility::time_f() - start;
					populate_result(_url, owner, rv, res);
				} else {
					rv.error = (http_error)res.error();
				}
			}
		}
		break;
		case m_patch: {
			if (auto res = cli.Patch(_url.c_str(), postdata.c_str(), mimetype.c_str())) {
				rv.latency = dpp::utility::time_f() - start;
				populate_result(_url, owner, rv, res);
			} else {
				rv.error = (http_error)res.error();
			}
		}
		break;
		case m_put: {
			/* PUT supports post data body */
			if (auto res = cli.Put(_url.c_str(), postdata.c_str(), mimetype.c_str())) {
				rv.latency = dpp::utility::time_f() - start;
				populate_result(_url, owner, rv, res);
			} else {
				rv.error = (http_error)res.error();
			}

		}
		break;
		case m_delete: {
			if (auto res = cli.Delete(_url.c_str())) {
				rv.latency = dpp::utility::time_f() - start;
				populate_result(_url, owner, rv, res);
			} else {
				rv.error = (http_error)res.error();
			}

		}
		break;
	}
	/* Set completion flag */
	completed = true;
	return rv;
}

request_queue::request_queue(class cluster* owner) : creator(owner), terminating(false), globally_ratelimited(false), globally_limited_for(0)
{
	in_thread = new std::thread(&request_queue::in_loop, this);
	out_thread = new std::thread(&request_queue::out_loop, this);
}

request_queue::~request_queue()
{
	creator->log(ll_debug, "REST request_queue shutting down");
	terminating = true;
	in_ready.notify_one();
	out_ready.notify_one();
	in_thread->join();
	out_thread->join();
	delete in_thread;
	delete out_thread;
}

void request_queue::in_loop()
{
	while (!terminating) {
		std::mutex mtx;
		std::unique_lock<std::mutex> lock{ mtx };			
		in_ready.wait_for(lock, std::chrono::seconds(1));
		/* New request to be sent! */

		if (!globally_ratelimited) {

			std::map<std::string, std::vector<http_request*>> requests_in_copy;
			{
				/* Make a safe copy within a mutex */
				std::lock_guard<std::mutex> lock(in_mutex);	
				requests_in_copy = requests_in;
			}

			for (auto & bucket : requests_in_copy) {
				for (auto req : bucket.second) {

					http_request_completion_t rv;
					auto currbucket = buckets.find(bucket.first);

					if (currbucket != buckets.end()) {
						/* There's a bucket for this request. Check its status. If the bucket says to wait,
						* skip all requests in this bucket till its ok.
						*/
						if (currbucket->second.remaining < 1) {
							uint64_t wait = (currbucket->second.retry_after ? currbucket->second.retry_after : currbucket->second.reset_after);
							if ((uint64_t)time(nullptr) > currbucket->second.timestamp + wait) {
								/* Time has passed, we can process this bucket again. send its request. */
								rv = req->run(creator);
							} else {
								/* Time not up yet, emit signal and wait */
								std::this_thread::sleep_for(std::chrono::milliseconds(50));
								in_ready.notify_one();
								break;
							}
						} else {
							/* There's limit remaining, we can just run the request */
							rv = req->run(creator);
						}
					} else {
						/* No bucket for this endpoint yet. Just send it, and make one from its reply */
						rv = req->run(creator);
					}

					bucket_t newbucket;
					newbucket.limit = rv.ratelimit_limit;
					newbucket.remaining = rv.ratelimit_remaining;
					newbucket.reset_after = rv.ratelimit_reset_after;
					newbucket.retry_after = rv.ratelimit_retry_after;
					newbucket.timestamp = time(NULL);
					globally_ratelimited = rv.ratelimit_global;
					if (globally_ratelimited) {
						globally_limited_for = (newbucket.retry_after ? newbucket.retry_after : newbucket.reset_after);
					}
					buckets[req->endpoint] = newbucket;

					/* Make a new entry in the completion list and notify */
					{
						std::lock_guard<std::mutex> lock(out_mutex);
						http_request_completion_t* hrc = new http_request_completion_t();
						*hrc = rv;
						responses_out.push(std::make_pair(hrc, req));
						out_ready.notify_one();
					}
				}
			}

			{
				std::lock_guard<std::mutex> lock(in_mutex);
				bool again = false;
				do {
					again = false;
					for (auto & bucket : requests_in) {
						for (auto req = bucket.second.begin(); req != bucket.second.end(); ++req) {
							if ((*req)->is_completed()) {
								requests_in[bucket.first].erase(req);
								again = true;
								goto out;	/* Only clean way out of a nested loop */
							}
						}
					}
					out:;
				} while (again);
			}

		} else {
			if (globally_limited_for > 0) {
				std::this_thread::sleep_for(std::chrono::seconds(globally_limited_for));
				globally_limited_for = 0;
			}
			globally_ratelimited = false;
			in_ready.notify_one();
		}
	}
	creator->log(ll_debug, "REST in-queue shutting down");
}

void request_queue::out_loop()
{
	while (!terminating) {

		std::mutex mtx;
		std::unique_lock<std::mutex> lock{ mtx };			
		out_ready.wait_for(lock, std::chrono::seconds(1));
		time_t now = time(nullptr);

		/* A request has been completed! */
		std::pair<http_request_completion_t*, http_request*> queue_head = {};
		{
			std::lock_guard<std::mutex> lock(out_mutex);
			if (responses_out.size()) {
				queue_head = responses_out.front();
				responses_out.pop();
			}
		}

		if (queue_head.first && queue_head.second) {
			queue_head.second->complete(*queue_head.first);
		}

		/* Queue deletions for 60 seconds from now */
		responses_to_delete.insert(std::make_pair(now + 60, queue_head));

		/* Check for deletable items every second regardless of select status */
		while (responses_to_delete.size() && now >= responses_to_delete.begin()->first) {
			delete responses_to_delete.begin()->second.first;
			delete responses_to_delete.begin()->second.second;
			responses_to_delete.erase(responses_to_delete.begin());
		}
	}
	creator->log(ll_debug, "REST out-queue shutting down");
}

/* Post a http_request into the queue */
void request_queue::post_request(http_request* req)
{
	std::lock_guard<std::mutex> lock(in_mutex);
	requests_in[req->endpoint].push_back(req);
	in_ready.notify_one();
}

std::string url_encode(const std::string &value) {
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex;

	for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
		std::string::value_type c = (*i);

		// Keep alphanumeric and other accepted characters intact
		if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~') {
			escaped << c;
			continue;
		}

		// Any other characters are percent-encoded
		escaped << std::uppercase;
		escaped << '%' << std::setw(2) << int((unsigned char) c);
		escaped << std::nouppercase;
	}

	return escaped.str();
}


};

