#include <dpp/queues.h>
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <dpp/httplib.h>
#include <dpp/stringops.h>

namespace dpp {

http_request::http_request(const cluster* owner, const std::string &_url, http_completion_event completion, const std::string &_postdata, http_method _method) : url(_url), complete_handler(completion), postdata(_postdata), method(_method), creator(owner)
{
}

http_request::~http_request() {
}

void http_request::complete(const http_request_completion_t &c) {
	if (complete_handler)
		complete_handler(c);
}

http_request_completion_t http_request::Run() {

	http_request_completion_t rv;

	httplib::Client cli("https://discord.com");
	cli.enable_server_certificate_verification(false);
	cli.set_follow_location(true);
	httplib::Headers headers = {
		{"Authorization", std::string("Bot ") + creator->token},
		{"User-Agent", "DiscordBot (https://github.com/brainboxdotcc/DPP, 0.0.1)"}
	};
	std::cout << std::string("Auth: Bot ") + creator->token << "\n";
	cli.set_default_headers(headers);

	rv.ratelimit_limit = rv.ratelimit_remaining = rv.ratelimit_reset = rv.ratelimit_retry_after = 0;
	rv.status = 0;
	
	switch (method) {
		case m_get: {
			if (auto res = cli.Get(url.c_str())) {
				rv.status = res->status;
				rv.body = (res->status < 400) ? res->body : "";
				for (auto &v : res->headers) {
					rv.headers[v.first] = v.second;
				}
				rv.ratelimit_limit = from_string<uint64_t>(res->get_header_value("X-RateLimit-Limit"), std::dec);
				rv.ratelimit_remaining = from_string<uint64_t>(res->get_header_value("X-RateLimit-Remaining"), std::dec);
				rv.ratelimit_reset = from_string<uint64_t>(res->get_header_value("X-RateLimit-Reset"), std::dec);
				if (res->get_header_value("X-RateLimit-Retry-After") != "") {
					rv.ratelimit_retry_after = from_string<uint64_t>(res->get_header_value("X-RateLimit-Retry-After"), std::dec);
				}
			} else {
				rv.error = (http_error)res.error();
			}
		}
		break;
		case m_post: {
			if (auto res = cli.Post(url.c_str(), postdata.c_str(), "application/json")) {
				rv.status = res->status;
				rv.body = (res->status < 400) ? res->body : "";
				for (auto &v : res->headers) {
					rv.headers[v.first] = v.second;
				}
				rv.ratelimit_limit = from_string<uint64_t>(res->get_header_value("X-RateLimit-Limit"), std::dec);
				rv.ratelimit_remaining = from_string<uint64_t>(res->get_header_value("X-RateLimit-Remaining"), std::dec);
				rv.ratelimit_reset = from_string<uint64_t>(res->get_header_value("X-RateLimit-Reset"), std::dec);
				if (res->get_header_value("X-RateLimit-Retry-After") != "") {
					rv.ratelimit_retry_after = from_string<uint64_t>(res->get_header_value("X-RateLimit-Retry-After"), std::dec);
				}
			} else {
				rv.error = (http_error)res.error();
			}
		}
		break;
		case m_patch: {
			if (auto res = cli.Patch(url.c_str())) {
				rv.status = res->status;
				rv.body = (res->status < 400) ? res->body : "";
				for (auto &v : res->headers) {
					rv.headers[v.first] = v.second;
				}
				rv.ratelimit_limit = from_string<uint64_t>(res->get_header_value("X-RateLimit-Limit"), std::dec);
				rv.ratelimit_remaining = from_string<uint64_t>(res->get_header_value("X-RateLimit-Remaining"), std::dec);
				rv.ratelimit_reset = from_string<uint64_t>(res->get_header_value("X-RateLimit-Reset"), std::dec);
				if (res->get_header_value("X-RateLimit-Retry-After") != "") {
					rv.ratelimit_retry_after = from_string<uint64_t>(res->get_header_value("X-RateLimit-Retry-After"), std::dec);
				}
			} else {
				rv.error = (http_error)res.error();
			}
		}
		break;
		case m_put: {
			if (auto res = cli.Put(url.c_str(), postdata.c_str(), "application/json")) {
				rv.status = res->status;
				rv.body = (res->status < 400) ? res->body : "";
				for (auto &v : res->headers) {
					rv.headers[v.first] = v.second;
				}
				rv.ratelimit_limit = from_string<uint64_t>(res->get_header_value("X-RateLimit-Limit"), std::dec);
				rv.ratelimit_remaining = from_string<uint64_t>(res->get_header_value("X-RateLimit-Remaining"), std::dec);
				rv.ratelimit_reset = from_string<uint64_t>(res->get_header_value("X-RateLimit-Reset"), std::dec);
				if (res->get_header_value("X-RateLimit-Retry-After") != "") {
					rv.ratelimit_retry_after = from_string<uint64_t>(res->get_header_value("X-RateLimit-Retry-After"), std::dec);
				}
			} else {
				rv.error = (http_error)res.error();
			}

		}
		break;
		case m_delete: {
			if (auto res = cli.Delete(url.c_str())) {
				rv.status = res->status;
				rv.body = (res->status < 400) ? res->body : "";
				for (auto &v : res->headers) {
					rv.headers[v.first] = v.second;
				}
				rv.ratelimit_limit = from_string<uint64_t>(res->get_header_value("X-RateLimit-Limit"), std::dec);
				rv.ratelimit_remaining = from_string<uint64_t>(res->get_header_value("X-RateLimit-Remaining"), std::dec);
				rv.ratelimit_reset = from_string<uint64_t>(res->get_header_value("X-RateLimit-Reset"), std::dec);
				if (res->get_header_value("X-RateLimit-Retry-After") != "") {
					rv.ratelimit_retry_after = from_string<uint64_t>(res->get_header_value("X-RateLimit-Retry-After"), std::dec);
				}
			} else {
				rv.error = (http_error)res.error();
			}

		}
		break;
	}
	return rv;
}


};
