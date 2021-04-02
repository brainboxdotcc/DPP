#pragma once
#include <unordered_map>
#include <string>
#include <queue>
#include <map>
#include <thread>
#include <mutex>
#include <vector>
#include <functional>

/* Queue of web requests, managed with rate limits */

namespace dpp {

enum http_error {
	h_success = 0,
	h_unknown,
	h_connection,
	h_bind_ip_address,
	h_read,
	h_write,
	h_exceed_redirect_count,
	h_canceled,
	h_ssl_connection,
	h_ssl_loading_certs,
	h_ssl_server_verification,
	h_unsupported_multipart_boundary_chars,
	h_compression,
};


struct http_request_completion_t {
	std::map<std::string, std::string> headers;
	uint16_t status = 0;
	http_error error = h_success;
	std::string ratelimit_bucket;
	uint64_t ratelimit_limit = 0;
	uint64_t ratelimit_remaining = 0;
	uint64_t ratelimit_reset_after = 0;
	uint64_t ratelimit_retry_after = 0;
	bool ratelimit_global = false;
	std::string body;
};

typedef std::function<void(const http_request_completion_t&)> http_completion_event;

enum http_method {
	m_get, m_post, m_put, m_patch, m_delete
};

class http_request {
	http_completion_event complete_handler;
	bool completed;
public:
	std::string endpoint;
	std::string parameters;
	std::string postdata;
	http_method method;

	http_request(const std::string &_endpoint, std::string &_parameters, http_completion_event completion, const std::string &_postdata = "", http_method method = m_get);
	~http_request();
	void complete(const http_request_completion_t &c);
	http_request_completion_t Run(const class cluster* owner);
	bool is_completed();
};

struct bucket_t {
	uint64_t limit;
	uint64_t remaining;
	uint64_t reset_after;
	uint64_t retry_after;
	time_t timestamp;
};

class request_queue {
private:
	const class cluster* creator;
	std::mutex in_mutex;
	std::mutex out_mutex;
	std::thread* in_thread;
	std::thread* out_thread;
	std::map<std::string, bucket_t> buckets;
	std::map<std::string, std::vector<http_request*>> requests_in;
	std::queue<std::pair<http_request_completion_t*, http_request*>> responses_out;
	bool terminating;
	bool globally_ratelimited;
	uint64_t globally_limited_for;

	int in_queue_port;
	int out_queue_port;
	int in_queue_listen_sock;
	int in_queue_connect_sock;
	int out_queue_listen_sock;
	int out_queue_connect_sock;

	void in_loop();
	void out_loop();
	void emit_in_queue_signal();
	void emit_out_queue_signal();
public:
	request_queue(const class cluster* owner);
	~request_queue();
	void post_request(http_request *req);
};

};
