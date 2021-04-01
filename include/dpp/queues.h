#pragma once
#include <unordered_map>
#include <string>
#include <queue>
#include <map>
#include <dpp/cluster.h>

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
	uint16_t status;
	http_error error;
	uint64_t ratelimit_limit;
	uint64_t ratelimit_remaining;
	uint64_t ratelimit_reset;
	uint64_t ratelimit_retry_after;
	std::string body;
};

typedef std::function<void(const http_request_completion_t&)> http_completion_event;

enum http_method {
	m_get, m_post, m_put, m_patch, m_delete
};

class http_request {
	http_completion_event complete_handler;
	const class cluster* creator;
public:
	std::string url;
	std::string postdata;
	http_method method;

	http_request(const class cluster* owner, const std::string &_url, http_completion_event completion, const std::string &_postdata = "", http_method method = m_get);
	~http_request();
	void complete(const http_request_completion_t &c);
	http_request_completion_t Run();
};

class request_queue {
};

};
