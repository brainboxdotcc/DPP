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

#include <dpp/export.h>

#if !DPP_BUILD_MODULES

#include <unordered_map>
#include <string>
#include <queue>
#include <map>
#include <shared_mutex>
#include <memory>
#include <vector>
#include <functional>
#include <atomic>
#include <dpp/httpsclient.h>

#endif

namespace dpp {

/**
 * @brief Error values. Most of these are currently unused in https_client.
 */
DPP_EXPORT enum http_error : uint8_t {
	/**
	 * @brief Request successful.
	 */
	h_success = 0,

	/**
	 * @brief Status unknown.
	 */
	h_unknown,

	/**
	 * @brief Connect failed.
	 */
	h_connection,

	/**
	 * @brief Invalid local ip address.
	 */
	h_bind_ip_address,

	/**
	 * @brief Read error.
	 */
	h_read,

	/**
	 * @brief Write error.
	 */
	h_write,

	/**
	 * @brief Too many 30x redirects.
	 */
	h_exceed_redirect_count,

	/**
	 * @brief Request cancelled.
	 */
	h_canceled,

	/**
	 * @brief SSL connection error.
	 */
	h_ssl_connection,

	/**
	 * @brief SSL cert loading error.
	 */
	h_ssl_loading_certs,

	/**
	 * @brief SSL server verification error.
	 */
	h_ssl_server_verification,

	/**
	 * @brief Unsupported multipart boundary characters.
	 */
	h_unsupported_multipart_boundary_chars,

	/**
	 * @brief Compression error.
	 */
	h_compression,
};

/**
 * @brief The result of any HTTP request. Contains the headers, vital
 * rate limit figures, and returned request body.
 */
DPP_EXPORT struct DPP_API http_request_completion_t {
	/**
	 * @brief HTTP headers of response.
	 */
	std::multimap<std::string, std::string> headers;

	/**
	 * @brief HTTP status.
	 * e.g. 200 = OK, 404 = Not found, 429 = Rate limited, etc.
	 */
	uint16_t status = 0;

	/**
	 * @brief Error status.
	 * e.g. if the request could not connect at all.
	 */
	http_error error = h_success;

	/**
	 * @brief Ratelimit bucket.
	 */
	std::string ratelimit_bucket;

	/**
	 * @brief Ratelimit limit of requests.
	 */
	uint64_t ratelimit_limit = 0;

	/**
	 * @brief Ratelimit remaining requests.
	 */
	uint64_t ratelimit_remaining = 0;

	/**
	 * @brief Ratelimit reset after (seconds).
	 */
	uint64_t ratelimit_reset_after = 0;

	/**
	 * @brief Ratelimit retry after (seconds).
	 */
	uint64_t ratelimit_retry_after = 0;

	/**
	 * @brief True if this request has caused us to be globally rate limited.
	 */
	bool ratelimit_global = false;

	/**
	 * @brief Reply body.
	 */
	std::string body;

	/**
	 * @brief Ping latency.
	 */
	double latency;
};

/**
 * @brief Results of HTTP requests are called back to these std::function types.
 *
 * @note Returned http_completion_events are called ASYNCHRONOUSLY in your
 * code which means they execute in a separate thread, results for the requests going
 * into a dpp::thread_pool. Completion events may not arrive in order depending on if
 * one request takes longer than another. Using the callbacks or using coroutines
 * correctly ensures that the order they arrive in the queue does not negatively affect
 * your code.
 */
DPP_EXPORT typedef std::function<void(const http_request_completion_t&)> http_completion_event;

/** 
 * @brief Various types of http method supported by the Discord API
 */
DPP_EXPORT enum http_method {
	/**
	 * @brief GET.
	 */
	m_get,

	/**
	 * @brief POST.
	 */
	m_post,

	/**
	 * @brief PUT.
	 */
	m_put,

	/**
	 * @brief PATCH.
	 */
	m_patch,

	/**
	 * @brief DELETE.
	 */
	m_delete
};

/**
 * @brief A HTTP request.
 * 
 * You should instantiate one of these objects via its constructor,
 * and pass a pointer to it into an instance of request_queue. Although you can
 * directly call the run() method of the object and it will make a HTTP call, be
 * aware that if you do this, it will be a **BLOCKING call** (not asynchronous) and
 * will not respect rate limits, as both of these functions are managed by the
 * request_queue class.
 */
DPP_EXPORT class DPP_API http_request {
	/**
	 * @brief Completion callback.
	 */
	http_completion_event complete_handler;

	/**
	 * @brief True if request has been made.
	 */
	bool completed;

	/**
	 * @brief True for requests that are not going to discord (rate limits code skipped).
	 */
	bool non_discord;

	/**
	 * @brief HTTPS client
	 */
	std::unique_ptr<https_client> cli;

public:
	/**
	 * @brief Endpoint name
	 * e.g. /api/users.
	 */
	std::string endpoint;

	/**
	 * @brief Major and minor parameters.
	 */
	std::string parameters;

	/**
	 * @brief Postdata for POST and PUT.
	 */
	std::string postdata;

	/**
	 * @brief HTTP method for request.
	 */
	http_method method;

	/**
	 * @brief Audit log reason for Discord requests, if non-empty.
	 */
	std::string reason;

	/**
	 * @brief Upload file name (server side).
	 */
	std::vector<std::string> file_name;

	/**
	 * @brief Upload file contents (binary).
	 */
	std::vector<std::string> file_content;

	/**
	 * @brief Upload file mime types.
	 * application/octet-stream if unspecified.
	 */
	std::vector<std::string> file_mimetypes;

	/**
	 * @brief Request mime type.
	 */
	std::string mimetype;

	/**
	 * @brief Request headers (non-discord requests only).
	 */
	std::multimap<std::string, std::string> req_headers;

	/**
	 * @brief Waiting for rate limit to expire.
	 */
	bool waiting;

	/**
	 * @brief HTTP protocol.
	 */
	std::string protocol;

	/**
	 * @brief Constructor. When constructing one of these objects it should be passed to request_queue::post_request().
	 * @param _endpoint The API endpoint, e.g. /api/guilds
	 * @param _parameters Major and minor parameters for the endpoint e.g. a user id or guild id
	 * @param completion completion event to call when done
	 * @param _postdata Data to send in POST and PUT requests
	 * @param method The HTTP method to use from dpp::http_method
	 * @param audit_reason Audit log reason to send, empty to send none
	 * @param filename The filename (server side) of any uploaded file
	 * @param filecontent The binary content of any uploaded file for the request
	 * @param filemimetype The MIME type of any uploaded file for the request
	 * @param http_protocol HTTP protocol
	 */
	http_request(const std::string &_endpoint, const std::string &_parameters, http_completion_event completion, const std::string &_postdata = "", http_method method = m_get, const std::string &audit_reason = "", const std::string &filename = "", const std::string &filecontent = "", const std::string &filemimetype = "", const std::string &http_protocol = "1.1");

	/**
	 * @brief Constructor. When constructing one of these objects it should be passed to request_queue::post_request().
	 * @param _endpoint The API endpoint, e.g. /api/guilds
	 * @param _parameters Major and minor parameters for the endpoint e.g. a user id or guild id
	 * @param completion completion event to call when done
	 * @param _postdata Data to send in POST and PUT requests
	 * @param method The HTTP method to use from dpp::http_method
	 * @param audit_reason Audit log reason to send, empty to send none
	 * @param filename The filename (server side) of any uploaded file
	 * @param filecontent The binary content of any uploaded file for the request
	 * @param filemimetypes The MIME type of any uploaded file for the request
	 * @param http_protocol HTTP protocol
	 */
	http_request(const std::string &_endpoint, const std::string &_parameters, http_completion_event completion, const std::string &_postdata = "", http_method method = m_get, const std::string &audit_reason = "", const std::vector<std::string> &filename = {}, const std::vector<std::string> &filecontent = {}, const std::vector<std::string> &filemimetypes = {}, const std::string &http_protocol = "1.1");

	/**
	 * @brief Constructor. When constructing one of these objects it should be passed to request_queue::post_request().
	 * @param _url Raw HTTP url
	 * @param completion completion event to call when done
	 * @param method The HTTP method to use from dpp::http_method
	 * @param _postdata Data to send in POST and PUT requests
	 * @param _mimetype POST data mime type
	 * @param _headers HTTP headers to send
	 * @param http_protocol HTTP protocol
	 */
	http_request(const std::string &_url, http_completion_event completion, http_method method = m_get, const std::string &_postdata = "", const std::string &_mimetype = "text/plain", const std::multimap<std::string, std::string> &_headers = {}, const std::string &http_protocol = "1.1");

	/**
	 * @brief Destroy the http request object
	 */
	~http_request();

	/**
	 * @brief Call the completion callback, if the request is complete.
	 * @param c callback to call
	 */
	void complete(const http_request_completion_t &c);

	/**
	 * @brief Execute the HTTP request and mark the request complete.
	 * @param processor Request concurrency queue this request was created by
	 * @param owner creating cluster
	 */
	http_request_completion_t run(class request_concurrency_queue* processor, class cluster* owner);

	/**
	 * @brief Returns true if the request is complete
	 * @return True if completed
	 * */
	bool is_completed();

	/**
	 * @brief Get the HTTPS client used to perform this request, or nullptr if there is none
	 * @return https_client object
	 */
	https_client* get_client() const;
};

/**
 * @brief A rate limit bucket. The library builds one of these for
 * each endpoint.
 */
DPP_EXPORT struct DPP_API bucket_t {
	/**
	 * @brief Request limit.
	 */
	uint64_t limit;

	/**
	 * @brief Requests remaining.
	 */
	uint64_t remaining;

	/**
	 * @brief Rate-limit of this bucket resets after this many seconds.
	 */
	uint64_t reset_after;

	/**
	 * @brief Rate-limit of this bucket can be retried after this many seconds.
	 */
	uint64_t retry_after;

	/**
	 * @brief Timestamp this buckets counters were updated.
	 */
	time_t timestamp;
};


/**
 * @brief Represents a timer instance in a pool handling requests to HTTP(S) servers.
 * There are several of these, the total defined by a constant in cluster.cpp, and each
 * one will always receive requests for the same rate limit bucket based on its endpoint
 * portion of the url. This makes rate limit handling reliable and easy to manage.
 * Each of these also has its own mutex, making it thread safe to call and use these
 * from anywhere in the code.
 */
DPP_EXPORT class DPP_API request_concurrency_queue {
public:
	/**
	 * @brief Queue index
	 */
	int in_index{0};

	/**
	 * @brief True if ending.
	 */
	std::atomic<bool> terminating;

	/**
	 * @brief Request queue that owns this request_concurrency_queue.
	 */
	class request_queue* requests;

	/**
	 * @brief The cluster that owns this request_concurrency_queue.
	 */
	cluster* creator;

	/**
	 * @brief Inbound queue mutex thread safety.
	 */
	std::shared_mutex in_mutex;

	/**
	 * @brief Inbound queue timer. The timer is called every second,
	 * and when it wakes up it checks for requests pending to be sent in the queue.
	 * If there are any requests and we are not waiting on rate limit, it will send them,
	 * else it will wait for the rate limit to expire.
	 */
	dpp::timer in_timer;

	/**
	 * @brief Rate-limit bucket counters.
	 */
	std::map<std::string, bucket_t> buckets;

	/**
	 * @brief Queue of requests to be made. Sorted by http_request::endpoint.
	 */
	std::vector<std::unique_ptr<http_request>> requests_in;

	/**
	 * @brief Requests to remove after a set amount of time has passed
	 */
	std::vector<std::unique_ptr<http_request>> removals;

	/**
	 * @brief Timer callback
	 * @param index Index ID for this timer
	 */
	void tick_and_deliver_requests(uint32_t index);

	/**
	 * @brief Construct a new concurrency queue object
	 * 
	 * @param owner Owning cluster
	 * @param req_q Owning request queue
	 * @param index Queue index number, uniquely identifies this queue for hashing
	 */
	request_concurrency_queue(cluster* owner, class request_queue* req_q, uint32_t index);

	/**
	 * @brief Destroy the concurrency queue object
	 * This will stop the timer.
	 */
	~request_concurrency_queue();

	/**
	 * @brief Flags the queue as terminating
	 * This will set the internal atomic bool that indicates this queue is to accept no more requests
	 */
	void terminate();

	/**
	 * @brief Post a http_request to this queue.
	 * 
	 * @param req http_request to post. The pointer will be freed when it has
	 * been executed.
	 */
	void post_request(std::unique_ptr<http_request> req);
};

/**
 * @brief The request_queue class manages rate limits and marshalls HTTP requests that have
 * been built as http_request objects.
 * 
 * It ensures asynchronous delivery of events and queueing of requests.
 *
 * It will spawn multiple timers to make outbound HTTP requests and then call the callbacks
 * of those requests on completion within the dpp::thread_pool for the cluster.
 * If the user decides to take a long time processing a reply in their callback it won't affect
 * when other requests are sent, and if a HTTP request takes a long time due to latency, it won't
 * hold up user processing.
 *
 * There are usually two request_queue objects in each dpp::cluster, one of which is used
 * internally for the various REST methods to Discord such as sending messages, and the other
 * used to support user REST calls via dpp::cluster::request(). They are separated so that the
 * one for user requests can be specifically configured to never ever send the Discord token
 * unless it is explicitly placed into the request, for security reasons.
 */
DPP_EXPORT class DPP_API request_queue {
public:
	/**
	 * @brief Required so request_concurrency_queue can access these member variables
	 */
	friend class request_concurrency_queue;

	/**
	 * @brief The cluster that owns this request_queue
	 */
	cluster* creator;

	/**
	 * @brief A completed request. Contains both the request and the response
	 */
	struct completed_request {
		/**
		 * @brief Request sent
		 */
		std::unique_ptr<http_request> request;

		/**
		 * @brief Response to the request
		 */
		std::unique_ptr<http_request_completion_t> response;
	};

	/**
	 * @brief A vector of timers forming a pool.
	 *
	 * There are a set number of these defined by a constant in cluster.cpp. A request is always placed
	 * on the same element in this vector, based upon its url, so that two conditions are satisfied:
	 *
	 * 1) Any requests for the same ratelimit bucket are handled by the same concurrency queue in the pool so that
	 * they do not create unnecessary 429 errors,
	 * 2) Requests for different endpoints go into different buckets, so that they may be requested in parallel
	 * A global ratelimit event pauses all timers in the pool. These are few and far between.
	 */
	std::vector<std::unique_ptr<request_concurrency_queue>> requests_in;

	/**
	 * @brief Set to true if the timers should terminate.
	 * When this is set to true no further requests are accepted to the queues.
	 */
	std::atomic<bool> terminating;

	/**
	 * @brief True if globally rate limited
	 *
	 * When globally rate limited the concurrency queues associated with this request queue
	 * will not process any requests in their timers until the global rate limit expires.
	 */
	bool globally_ratelimited;

	/**
	 * @brief When we are globally rate limited until (unix epoch)
	 *
	 * @note Only valid if globally_rate limited is true. If we are globally rate limited,
	 * queues in this class will not process requests until the current unix epoch time
	 * is greater than this time.
	 */
	time_t globally_limited_until;

	/**
	 * @brief Number of request queues in the pool. This is the direct size of the requests_in
	 * vector.
	 */
	uint32_t in_queue_pool_size;

	/**
	 * @brief constructor
	 * @param owner The creating cluster.
	 * @param request_concurrency The number of http request queues to allocate.
	 * Each request queue is a dpp::timer which ticks every second looking for new
	 * requests to run. The timer will hold back requests if we are waiting as to comply
	 * with rate limits. Adding a request to this class will cause the queue it is placed in
	 * to run immediately but this cannot override rate limits.
	 * By default eight concurrency queues are allocated.
	 * Side effects: Creates timers for the queue
	 */
	request_queue(cluster* owner, uint32_t request_concurrency = 8);

	/**
	 * @brief Get the request queue concurrency count
	 * @return uint32_t number of request queues that are active
	 */
	uint32_t get_request_queue_count() const;

	/**
	 * @brief Destroy the request queue object.
	 * Side effects: Ends and deletes concurrency timers
	 */
	~request_queue();

	/**
	 * @brief Put a http_request into the request queue.
	 * @note Will use a simple hash function to determine which of the 'in queues' to place
	 * this request onto.
	 * @param req request to add
	 * @return reference to self
	 */
	request_queue& post_request(std::unique_ptr<http_request> req);

	/**
	 * @brief Returns true if the bot is currently globally rate limited
	 * @return true if globally rate limited
	 */
	bool is_globally_ratelimited() const;

	/**
	 * @brief Returns the number of active requests on this queue
	 * @return Total number of active requests
	 */
	size_t get_active_request_count() const;
};

}
