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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <io.h>
#pragma comment(lib,"ws2_32")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#endif

#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <random>
#include <dpp/queues.h>
#include <dpp/cluster.h>
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <dpp/httplib.h>
#include <dpp/fmt/format.h>
#include <dpp/stringops.h>
#include <dpp/version.h>

namespace dpp {

static std::string http_version = "DiscordBot (https://github.com/brainboxdotcc/DPP, " + std::to_string(DPP_VERSION_MAJOR) + "." + std::to_string(DPP_VERSION_MINOR) + "." + std::to_string(DPP_VERSION_PATCH) + ")";

http_request::http_request(const std::string &_endpoint, const std::string &_parameters, http_completion_event completion, const std::string &_postdata, http_method _method, const std::string &filename, const std::string &filecontent) : endpoint(_endpoint), parameters(_parameters), complete_handler(completion), postdata(_postdata), method(_method), completed(false), file_name(filename), file_content(filecontent)
{
}

http_request::~http_request() {
}

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
	rv.ratelimit_limit = from_string<uint64_t>(res->get_header_value("X-RateLimit-Limit"), std::dec);
	rv.ratelimit_remaining = from_string<uint64_t>(res->get_header_value("X-RateLimit-Remaining"), std::dec);
	rv.ratelimit_reset_after = from_string<uint64_t>(res->get_header_value("X-RateLimit-Reset-After"), std::dec);
	rv.ratelimit_bucket = res->get_header_value("X-RateLimit-Bucket");
	rv.ratelimit_global = (res->get_header_value("X-RateLimit-Global") == "true");
	owner->rest_ping = rv.latency;
	if (res->get_header_value("X-RateLimit-Retry-After") != "") {
		rv.ratelimit_retry_after = from_string<uint64_t>(res->get_header_value("X-RateLimit-Retry-After"), std::dec);
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
http_request_completion_t http_request::Run(cluster* owner) {

	http_request_completion_t rv;
	double start = dpp::utility::time_f();

	httplib::Client cli("https://discord.com");
	/* This is for a reason :( - Some systems have really out of date cert stores */
	cli.enable_server_certificate_verification(false);
	cli.set_follow_location(true);
	/* TODO: Once we have a version number header, use it here */
	httplib::Headers headers = {
		{"Authorization", std::string("Bot ") + owner->token},
		{"User-Agent", http_version}
	};
	cli.set_default_headers(headers);

	rv.ratelimit_limit = rv.ratelimit_remaining = rv.ratelimit_reset_after = rv.ratelimit_retry_after = 0;
	rv.status = 0;
	rv.latency = 0;
	rv.ratelimit_global = false;

	std::string _url = endpoint;
	if (!empty(parameters)) {
		_url = endpoint + "/" +parameters;
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
				httplib::MultipartFormDataItems items = {
					{ "payload_json", postdata, "", "application/json" },
					{ "file", file_content, file_name, "application/octet-stream" }
				};
				if (auto res = cli.Post(_url.c_str(), items)) {
					rv.latency = dpp::utility::time_f() - start;
					populate_result(_url, owner, rv, res);
				} else {
					rv.error = (http_error)res.error();
				}
			} else {
				if (auto res = cli.Post(_url.c_str(), postdata.c_str(), "application/json")) {
					rv.latency = dpp::utility::time_f() - start;
					populate_result(_url, owner, rv, res);
				} else {
					rv.error = (http_error)res.error();
				}
			}
		}
		break;
		case m_patch: {
			if (auto res = cli.Patch(_url.c_str(), postdata.c_str(), "application/json")) {
				rv.latency = dpp::utility::time_f() - start;
				populate_result(_url, owner, rv, res);
			} else {
				rv.error = (http_error)res.error();
			}
		}
		break;
		case m_put: {
			/* PUT supports post data body */
			if (auto res = cli.Put(_url.c_str(), postdata.c_str(), "application/json")) {
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
	in_queue_listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	out_queue_listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (in_queue_listen_sock == -1 || out_queue_listen_sock == -1) {
		throw dpp::exception("Can't initialise request queue sockets");
	}

	std::mt19937 generator(time(NULL));
	std::uniform_real_distribution<double> distribution(8192, 32760);

	in_queue_port = distribution(generator);
	out_queue_port = distribution(generator);

	struct sockaddr_in in_server, out_server;
	in_server.sin_family = out_server.sin_family = AF_INET;
	in_server.sin_addr.s_addr = out_server.sin_addr.s_addr = htonl(0x7f000001); /* Localhost */
	in_server.sin_port = htons(in_queue_port);
	out_server.sin_port = htons(out_queue_port);

	if ((bind(in_queue_listen_sock, (struct sockaddr *)&in_server , sizeof(in_server)) < 0) || (bind(out_queue_listen_sock, (struct sockaddr *)&out_server , sizeof(out_server)) < 0)) {
		throw dpp::exception("Can't bind request queue sockets");
	}
	/* Backlog is only 1, because we only expect our own system to connect back to this once */
	if (listen(in_queue_listen_sock, 1) == -1 || listen(out_queue_listen_sock, 1) == -1) {
		throw dpp::exception("Can't listen() on request queue sockets");
	}

	in_thread = new std::thread(&request_queue::in_loop, this);
	out_thread = new std::thread(&request_queue::out_loop, this);

	std::this_thread::sleep_for(std::chrono::milliseconds(250));	

	in_queue_connect_sock = socket(AF_INET, SOCK_STREAM, 0);
	out_queue_connect_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (in_queue_connect_sock == -1 || out_queue_connect_sock == -1) {
		throw dpp::exception("Can't initialise request queue notifier sockets");
	}

	struct sockaddr_in in_client, out_client;
	in_client.sin_addr.s_addr = out_client.sin_addr.s_addr = inet_addr("127.0.0.1");
	in_client.sin_family = out_client.sin_family = AF_INET;
	in_client.sin_port = htons(in_queue_port);
	out_client.sin_port = htons(out_queue_port);

	if ((connect(in_queue_connect_sock, (struct sockaddr *)&in_client, sizeof(in_client)) < 0) || (connect(out_queue_connect_sock, (struct sockaddr *)&out_client, sizeof(out_client)) < 0)) {
		throw dpp::exception("Can't connect notifiers");
	}
}

request_queue::~request_queue()
{
	creator->log(ll_debug, "REST request_queue shutting down");
	terminating = true;
	in_thread->join();
	out_thread->join();
	delete in_thread;
	delete out_thread;
}

void request_queue::in_loop()
{
	int c = sizeof(struct sockaddr_in);
	fd_set readfds;
	timeval ts;
	char n;
	struct sockaddr_in client;
	int notifier = accept(in_queue_listen_sock, (struct sockaddr *)&client, (socklen_t*)&c);
	if (notifier < 0) {
		throw dpp::exception("Failed to initialise REST in queue - can't accept notifier connection");
	}
#ifndef _WIN32
	close(in_queue_listen_sock);
#endif
	while (!terminating) {
		/* select for one second, waiting for new data */
		FD_ZERO(&readfds);
		FD_SET(notifier, &readfds);
		ts.tv_sec = 1;
		ts.tv_usec = 0;
		int r = select(FD_SETSIZE, &readfds, 0, 0, &ts);
		time_t now = time(nullptr);

		if (r > 0 && FD_ISSET(notifier, &readfds)) {

			if (recv(notifier, &n, 1, 0) > 0) {
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
									if (time(NULL) > currbucket->second.timestamp + wait) {
										/* Time has passed, we can process this bucket again. send its request. */
										rv = req->Run(creator);
									} else {
										/* Time not up yet, emit signal and wait */
										std::this_thread::sleep_for(std::chrono::milliseconds(50));
										emit_in_queue_signal();
										break;
									}
								} else {
									/* There's limit remaining, we can just run the request */
									rv = req->Run(creator);
								}
							} else {
								/* No bucket for this endpoint yet. Just send it, and make one from its reply */
								rv = req->Run(creator);
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
								emit_out_queue_signal();
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
					emit_in_queue_signal();
				}
			}
		}
	}
	creator->log(ll_debug, "REST in-queue shutting down");
	shutdown(notifier, 2);
	#ifdef _WIN32
		if (sfd >= 0 && sfd < FD_SETSIZE) {
			closesocket(notifier);
		}
	#else
		::close(notifier);
	#endif
}

void request_queue::out_loop()
{
	int c = sizeof(struct sockaddr_in);
	fd_set readfds;
	timeval ts;
	char n;
	struct sockaddr_in client;
	SOCKET notifier = accept(out_queue_listen_sock, (struct sockaddr *)&client, (socklen_t*)&c);
	if (notifier < 0) {
		throw dpp::exception("Failed to initialise REST out queue - can't accept notifier connection");
	}
#ifndef _WIN32
	close(out_queue_listen_sock);
#endif
	while (!terminating) {

		/* select for one second, waiting for new data */
		FD_ZERO(&readfds);
		FD_SET(notifier, &readfds);
		ts.tv_sec = 1;
		ts.tv_usec = 0;
		int r = select(FD_SETSIZE, &readfds, 0, 0, &ts);
		time_t now = time(nullptr);

		if (r > 0 && FD_ISSET(notifier, &readfds)) {
			if (recv(notifier, &n, 1, 0) > 0) {

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
			}
		}

		/* Check for deletable items every second regardless of select status */
		while (responses_to_delete.size() && now >= responses_to_delete.begin()->first) {
			delete responses_to_delete.begin()->second.first;
			delete responses_to_delete.begin()->second.second;
			responses_to_delete.erase(responses_to_delete.begin());
		}
	}
	creator->log(ll_debug, "REST out-queue shutting down");
	shutdown(notifier, 2);
	#ifdef _WIN32
		if (notifier >= 0 && notifier < FD_SETSIZE) {
			closesocket(notifier);
		}
	#else
		::close(notifier);
	#endif
}


/* These only need to send a byte to notify the other end of something to do. any byte will do.
 */
void request_queue::emit_in_queue_signal()
{
	send(in_queue_connect_sock, "X", 1, 0);
}

void request_queue::emit_out_queue_signal()
{
	send(out_queue_connect_sock, "X", 1, 0);
}

/* Post a http_request into the queue */
void request_queue::post_request(http_request* req)
{
	std::lock_guard<std::mutex> lock(in_mutex);
	requests_in[req->endpoint].push_back(req);
	emit_in_queue_signal();
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

