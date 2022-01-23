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
#pragma once
#include <dpp/export.h>
#include <unordered_map>
#include <string>
#include <queue>
#include <map>
#include <thread>
#include <mutex>
#include <vector>
#include <functional>
#include <condition_variable>

namespace dpp {

/**
 * @brief Encodes a url parameter similar to php urlencode()
 * 
 * @param value String to encode
 * @return * std::string URL encoded string
 */
std::string url_encode(const std::string &value);

/**
 * @brief Error values. Don't change the order or add extra values here,
 * as they map onto the error values of cpp-httplib
 */
enum http_error {
	/// Request successful
	h_success = 0,
	/// Status unknown
	h_unknown,
	/// Connect failed
	h_connection,
	/// Invalid local ip address
	h_bind_ip_address,
	/// Read error
	h_read,
	/// Write error
	h_write,
	/// Too many 30x redirects
	h_exceed_redirect_count,
	/// Request cancelled
	h_canceled,
	/// SSL connection error
	h_ssl_connection,
	/// SSL cert loading error
	h_ssl_loading_certs,
	/// SSL server verification error
	h_ssl_server_verification,
	/// Unsupported multipart boundary characters
	h_unsupported_multipart_boundary_chars,
	/// Compression error
	h_compression,
};

/**
 * @brief The result of any HTTP request. Contains the headers, vital
 * rate limit figures, and returned request body.
 */
struct DPP_EXPORT http_request_completion_t {
	/** HTTP headers of response */
	std::map<std::string, std::string> headers;
	/** HTTP status, e.g. 200 = OK, 404 = Not found, 429 = Rate limited */
	uint16_t status = 0;
	/** Error status (e.g. if the request could not connect at all) */
	http_error error = h_success;
	/** Ratelimit bucket */
	std::string ratelimit_bucket;
	/** Ratelimit limit of requests */
	uint64_t ratelimit_limit = 0;
	/** Ratelimit remaining requests */
	uint64_t ratelimit_remaining = 0;
	/** Ratelimit reset after (seconds) */
	uint64_t ratelimit_reset_after = 0;
	/** Ratelimit retry after (seconds) */
	uint64_t ratelimit_retry_after = 0;
	/** True if this request has caused us to be globally rate limited */
	bool ratelimit_global = false;
	/** Reply body */
	std::string body;
	/** Ping latency */
	double latency;
};

/**
 * @brief Results of HTTP requests are called back to these std::function types.
 * @note Returned http_completion_events are called ASYNCRONOUSLY in your
 * code which means they execute in a separate thread. The completion events
 * arrive in order.
 */
typedef std::function<void(const http_request_completion_t&)> http_completion_event;

/** Various types of http method supported by the Discord API
 */
enum http_method {
	/// GET
	m_get,
	/// POST
	m_post,
	/// PUT
	m_put,
	/// PATCH
	m_patch,
	/// DELETE
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
class DPP_EXPORT http_request {
	/** Completion callback */
	http_completion_event complete_handler;
	/** True if request has been made */
	bool completed;
	/** True for requests that are not going to discord (rate limits code skipped) */
	bool non_discord;
public:
	/** Endpoint name e.g. /api/users */
	std::string endpoint;
	/** Major and minor parameters */
	std::string parameters;
	/** Postdata for POST and PUT */
	std::string postdata;
	/** HTTP method for request */
	http_method method;
	/** Audit log reason for Discord requests, if non-empty */
	std::string reason;
	/** Upload file name (server side) */
	std::vector<std::string> file_name;
	/** Upload file contents (binary) */
	std::vector<std::string> file_content;
	/** Request mime type */
	std::string mimetype;
	/** Request headers (non-discord requests only) */
	std::multimap<std::string, std::string> req_headers;

	/** Constructor. When constructing one of these objects it should be passed to request_queue::post_request().
	 * @param _endpoint The API endpoint, e.g. /api/guilds
	 * @param _parameters Major and minor parameters for the endpoint e.g. a user id or guild id
	 * @param completion completion event to call when done
	 * @param _postdata Data to send in POST and PUT requests
	 * @param method The HTTP method to use from dpp::http_method
	 * @param audit_reason Audit log reason to send, empty to send none
	 * @param filename The filename (server side) of any uploaded file
	 * @param filecontent The binary content of any uploaded file for the request
	 */
	http_request(const std::string &_endpoint, const std::string &_parameters, http_completion_event completion, const std::string &_postdata = "", http_method method = m_get, const std::string &audit_reason = "", const std::string &filename = "", const std::string &filecontent = "");

	/** Constructor. When constructing one of these objects it should be passed to request_queue::post_request().
	 * @param _endpoint The API endpoint, e.g. /api/guilds
	 * @param _parameters Major and minor parameters for the endpoint e.g. a user id or guild id
	 * @param completion completion event to call when done
	 * @param _postdata Data to send in POST and PUT requests
	 * @param method The HTTP method to use from dpp::http_method
	 * @param audit_reason Audit log reason to send, empty to send none
	 * @param filename The filename (server side) of any uploaded file
	 * @param filecontent The binary content of any uploaded file for the request
	 */
	http_request(const std::string &_endpoint, const std::string &_parameters, http_completion_event completion, const std::string &_postdata = "", http_method method = m_get, const std::string &audit_reason = "", const std::vector<std::string> &filename = {}, const std::vector<std::string> &filecontent = {});

	/** Constructor. When constructing one of these objects it should be passed to request_queue::post_request().
	 * @param _url Raw HTTP url
	 * @param completion completion event to call when done
	 * @param method The HTTP method to use from dpp::http_method
	 * @param _postdata Data to send in POST and PUT requests
	 * @param _mimetype POST data mime type
	 * @param _headers HTTP headers to send
	 */
	http_request(const std::string &_url, http_completion_event completion, http_method method = m_get, const std::string &_postdata = "", const std::string &_mimetype = "text/plain", const std::multimap<std::string, std::string> &_headers = {});

	/** Destructor */
	~http_request();

	/** Call the completion callback, if the request is complete.
	 * @param c callback to call
	 */
	void complete(const http_request_completion_t &c);

	/** Execute the HTTP request and mark the request complete.
	 * @param owner creating cluster
	 */
	http_request_completion_t run(class cluster* owner);

	/** Returns true if the request is complete */
	bool is_completed();
};

/**
 * @brief A rate limit bucket. The library builds one of these for
 * each endpoint.
 */
struct DPP_EXPORT bucket_t {
	/** Request limit */
	uint64_t limit;
	/** Requests remaining */
	uint64_t remaining;
	/** Ratelimit of this bucket resets after this many seconds */
	uint64_t reset_after;
	/** Ratelimit of this bucket can be retried after this many seconds */
	uint64_t retry_after;
	/** Timestamp this buckets counters were updated */
	time_t timestamp;
};

/**
 * @brief The request_queue class manages rate limits and marshalls HTTP requests that have
 * been built as http_request objects.
 * 
 * It ensures asynchronous delivery of events and queueing of requests.
 *
 * It will spawn two threads, one to make outbound HTTP requests and push the returned
 * results into a queue, and the second to call the callback methods with these results.
 * They are separated so that if the user decides to take a long time processing a reply
 * in their callback it won't affect when other requests are sent, and if a HTTP request
 * takes a long time due to latency, it won't hold up user processing.
 *
 * There are usually two request_queue objects in each dpp::cluster, one of which is used
 * internally for the various REST methods to Discord such as sending messages, and the other
 * used to support user REST calls via dpp::cluster::request().
 */
class DPP_EXPORT request_queue {
private:
	/** The cluster that owns this request_queue */
	class cluster* creator;

	/** Inbound queue mutex thread safety */
	std::mutex in_mutex;

	/** Outbound queue mutex thread safety */
	std::mutex out_mutex;

	/** Inbound queue thread */
	std::thread* in_thread;

	/** Outbound queue thread */
	std::thread* out_thread;

	/** Inbound queue condition, signalled when there are requests to fulfill */
	std::condition_variable in_ready;

	/** Outbound queue condition, signalled when there are requests completed to call callbacks for */ 
	std::condition_variable out_ready;

	/** Ratelimit bucket counters */
	std::map<std::string, bucket_t> buckets;

	/** Queue of requests to be made */
	std::map<std::string, std::vector<http_request*>> requests_in;

	/** Completed requests queue */
	std::queue<std::pair<http_request_completion_t*, http_request*>> responses_out;

	/** Completed requests to delete */
	std::multimap<time_t, std::pair<http_request_completion_t*, http_request*>> responses_to_delete;

	/** Set to true if the threads should terminate */
	bool terminating;

	/** True if globally rate limited - makes the entire request thread wait */
	bool globally_ratelimited;

	/** How many seconds we are globally rate limited for, if globally_ratelimited is true */
	uint64_t globally_limited_for;

	/**
	 * @brief Inbound queue thread loop
	 */
	void in_loop();

	/**
	 * @brief Outbound queue thread loop
	 */
	void out_loop();
public:

	/** Constructor
	 * @param owner The creating cluster.
	 * Side effects: Creates two threads for the queue
	 */
	request_queue(class cluster* owner);

	/**
	 * @brief Destroy the request queue object.
	 * Side effects: Joins and deletes queue threads
	 */
	~request_queue();

	/**
	 * @brief Put a http_request into the request queue. You should ALWAYS "new" an object
	 * to pass to here -- don't submit an object that's on the stack!
	 * @param req request to add
	 */
	void post_request(http_request *req);
};

};
