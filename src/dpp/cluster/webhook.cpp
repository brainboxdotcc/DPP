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
#include <dpp/webhook.h>
#include <dpp/cluster.h>
#include <dpp/nlohmann/json.hpp>

namespace dpp {

void cluster::webhook_create(const class webhook &w, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(w.channel_id), "webhooks", m_post, w.build_json(false), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("webhook", webhook().fill_from_json(&j), http));
		}
	});
}


void cluster::webhook_delete(snowflake webhook_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/webhooks", std::to_string(webhook_id), "", m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::webhook_message_delete(const class webhook &wh, snowflake message_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/webhooks", std::to_string(wh.id), dpp::url_encode(!wh.token.empty() ? wh.token: token) + "/messages/" + std::to_string(message_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::webhook_with_token_delete(snowflake webhook_id, const std::string &token, command_completion_event_t callback) {
	this->post_rest(API_PATH "/webhooks", std::to_string(webhook_id), dpp::url_encode(token), m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::webhook_edit(const class webhook& wh, command_completion_event_t callback) {
	this->post_rest(API_PATH "/webhooks", std::to_string(wh.id), "", m_patch, wh.build_json(true), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("webhook", webhook().fill_from_json(&j), http));
		}
	});
}


void cluster::webhook_message_edit(const class webhook &wh, const struct message& m, command_completion_event_t callback) {
	this->post_rest(API_PATH "/webhooks", std::to_string(wh.id), dpp::url_encode(!wh.token.empty() ? wh.token: token) + "/messages/" + std::to_string(m.id), m_patch, m.build_json(false), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("message", message().fill_from_json(&j), http));
		}
	});
}


void cluster::webhook_with_token_edit(const class webhook& wh, command_completion_event_t callback) {
	json jwh = json::parse(wh.build_json(true));
	if (jwh.find("channel_id") != jwh.end()) {
		jwh.erase(jwh.find("channel_id"));
	}
	this->post_rest(API_PATH "/webhooks", std::to_string(wh.id), dpp::url_encode(wh.token), m_patch, jwh.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("webhook", webhook().fill_from_json(&j), http));
		}
	});
}


void cluster::webhook_execute(const class webhook &wh, const struct message& m, bool wait, snowflake thread_id, command_completion_event_t callback) {
	std::string parameters;
	if (wait) {
		parameters.append("&wait=true");
	}
	if (thread_id) {
		parameters.append("&thread_id=" + std::to_string(thread_id));
	}
	if (!parameters.empty()) {
		parameters[0] = '?';
	}
	this->post_rest(API_PATH "/webhooks", std::to_string(wh.id), dpp::url_encode(!wh.token.empty() ? wh.token: token) + parameters, m_post, m.build_json(false), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("message", message().fill_from_json(&j), http));
		}
	});
}


void cluster::channel_webhooks_get(snowflake channel_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "webhooks", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			webhook_map webhooks;
			confirmation_callback_t e("confirmation", confirmation(), http);
			if (!e.is_error()) {
				for (auto & curr_webhook: j) {
					webhooks[snowflake_not_null(&curr_webhook, "id")] = webhook().fill_from_json(&curr_webhook);
				}
			}
			callback(confirmation_callback_t("webhook_map", webhooks, http));
		}
	});
}


void cluster::guild_webhooks_get(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "webhooks", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			webhook_map webhooks;
			confirmation_callback_t e("confirmation", confirmation(), http);
			if (!e.is_error()) {
				for (auto & curr_webhook: j) {
					webhooks[snowflake_not_null(&curr_webhook, "id")] = webhook().fill_from_json(&curr_webhook);
				}
			}
			callback(confirmation_callback_t("webhook_map", webhooks, http));
		}
	});
}

void cluster::webhook_get(snowflake webhook_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/webhooks", std::to_string(webhook_id), "", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("webhook", webhook().fill_from_json(&j), http));
		}
	});
}


void cluster::webhook_message_get(const class webhook &wh, command_completion_event_t callback)
{
	this->post_rest(API_PATH "/webhooks", std::to_string(wh.id), dpp::url_encode(!wh.token.empty() ? wh.token: token) + "/messages/@original", m_get, "", [callback](json &j, const http_request_completion_t &http){
		if (callback){
			callback(confirmation_callback_t("message", message().fill_from_json(&j), http));
		}
	});
}


void cluster::webhook_with_token_get(snowflake webhook_id, const std::string &token, command_completion_event_t callback) {
	this->post_rest(API_PATH "/webhooks", std::to_string(webhook_id), dpp::url_encode(token), m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("webhook", webhook().fill_from_json(&j), http));
		}
	});
}

};