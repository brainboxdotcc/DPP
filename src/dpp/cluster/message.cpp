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
#include <dpp/cluster.h>
#include <dpp/nlohmann/json.hpp>

namespace dpp {

void cluster::message_add_reaction(const struct message &m, const std::string &reaction, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(m.channel_id), "messages/" + std::to_string(m.id) + "/reactions/" + dpp::url_encode(reaction) + "/@me", m_put, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::message_add_reaction(snowflake message_id, snowflake channel_id, const std::string &reaction, command_completion_event_t callback) {
	message m(channel_id, "");
	m.id = message_id;
	message_add_reaction(m, reaction, callback);
}



void cluster::message_create(const message &m, command_completion_event_t callback) {
	this->post_rest_multipart(API_PATH "/channels", std::to_string(m.channel_id), "messages", m_post, m.build_json(), [this, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("message", message(this).fill_from_json(&j), http));
		}
	}, m.filename, m.filecontent);
}


void cluster::message_crosspost(snowflake message_id, snowflake channel_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "messages/" + std::to_string(message_id) + "/crosspost", m_post, "", [this, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("message", message(this).fill_from_json(&j), http));
		}
	});
}


void cluster::message_delete_all_reactions(const struct message &m, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(m.channel_id), "messages/" + std::to_string(m.id) + "/reactions",  m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::message_delete_all_reactions(snowflake message_id, snowflake channel_id, command_completion_event_t callback) {
	message m(channel_id, "");
	m.id = message_id;
	m.owner = this;
	message_delete_all_reactions(m, callback);
}


void cluster::message_delete_bulk(const std::vector<snowflake>& message_ids, snowflake channel_id, command_completion_event_t callback) {
	json j;
	for (auto & m : message_ids) {
		j["messages"].push_back(std::to_string(m));
	}
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "messages/bulk-delete", m_post, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::message_delete(snowflake message_id, snowflake channel_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "messages/" + std::to_string(message_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::message_delete_own_reaction(const struct message &m, const std::string &reaction, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(m.channel_id), "messages/" + std::to_string(m.id) + "/reactions/" + dpp::url_encode(reaction) + "/@me", m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::message_delete_own_reaction(snowflake message_id, snowflake channel_id, const std::string &reaction, command_completion_event_t callback) {
	message m(channel_id, "");
	m.id = message_id;
	m.owner = this;
	message_delete_own_reaction(m, reaction, callback);
}


void cluster::message_delete_reaction(const struct message &m, snowflake user_id, const std::string &reaction, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(m.channel_id), "messages/" + std::to_string(m.id) + "/reactions/" + dpp::url_encode(reaction) + "/" + std::to_string(user_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::message_delete_reaction(snowflake message_id, snowflake channel_id, snowflake user_id, const std::string &reaction, command_completion_event_t callback) {
	message m(channel_id, "");
	m.id = message_id;
	m.owner = this;
	message_delete_reaction(m, user_id, reaction, callback);
}


void cluster::message_delete_reaction_emoji(const struct message &m, const std::string &reaction, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(m.channel_id), "messages/" + std::to_string(m.id) + "/reactions/" + dpp::url_encode(reaction), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::message_delete_reaction_emoji(snowflake message_id, snowflake channel_id, const std::string &reaction, command_completion_event_t callback) {
	message m(channel_id, "");
	m.id = message_id;
	m.owner = this;
	message_delete_reaction_emoji(m, reaction, callback);
}


void cluster::message_edit(const message &m, command_completion_event_t callback) {
	this->post_rest_multipart(API_PATH "/channels", std::to_string(m.channel_id), "messages/" + std::to_string(m.id), m_patch, m.build_json(true), [this, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("message", message(this).fill_from_json(&j), http));
		}
	}, m.filename, m.filecontent);
}


void cluster::message_get(snowflake message_id, snowflake channel_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "messages/" + std::to_string(message_id), m_get, "", [this, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("message", message(this).fill_from_json(&j), http));
		}
	});
}


void cluster::message_get_reactions(const struct message &m, const std::string &reaction, snowflake before, snowflake after, snowflake limit, command_completion_event_t callback) {
	std::string parameters;
	if (before) {
		parameters.append("&before=" + std::to_string(before));
	}
	if (after) {
		parameters.append("&after=" + std::to_string(after));
	}
	if (limit) {
		parameters.append("&limit=" + std::to_string(limit));
	}
	if (!parameters.empty()) {
		parameters[0] = '?';
	}
	this->post_rest(API_PATH "/channels", std::to_string(m.channel_id), "messages/" + std::to_string(m.id) + "/reactions/" + dpp::url_encode(reaction) + parameters, m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			user_map users;
			confirmation_callback_t e("confirmation", confirmation(), http);
			if (!e.is_error()) {
				for (auto & curr_user : j) {
					users[snowflake_not_null(&curr_user, "id")] = user().fill_from_json(&curr_user);
				}
			}
			callback(confirmation_callback_t("user_map", users, http));
		}
	});
}

void cluster::message_get_reactions(snowflake message_id, snowflake channel_id, const std::string &reaction, snowflake before, snowflake after, snowflake limit, command_completion_event_t callback) {
	message m(channel_id, "");
	m.id = message_id;
	m.owner = this;
	message_get_reactions(m, reaction, before, after, limit, callback);
}


void cluster::message_pin(snowflake channel_id, snowflake message_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "pins/" + std::to_string(message_id), m_put, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

thread_local message_map big_mm;

void cluster::messages_get(snowflake channel_id, snowflake around, snowflake before, snowflake after, uint32_t limit, command_completion_event_t callback) {
	std::function impl = [&](snowflake channel_id, snowflake around, snowflake before, snowflake after, uint32_t limit, command_completion_event_t callback) {
		std::string parameters;
		if (around) {
			parameters.append("&around=" + std::to_string(around));
		}
		if (before) {
			parameters.append("&before=" + std::to_string(before));
		}
		if (after) {
			parameters.append("&after=" + std::to_string(after));
		}
		if (limit) {
			if (limit > 100) {
				limit = 100;
			}
			parameters.append("&limit=" + std::to_string(limit));
		}
		if (!parameters.empty()) {
			parameters[0] = '?';
		}
		this->post_rest(API_PATH "/channels", std::to_string(channel_id), "messages" + parameters, m_get, "", [this, callback](json &j, const http_request_completion_t& http) {
			if (callback) {
				confirmation_callback_t e("confirmation", confirmation(), http);
				message_map messages;
				if (!e.is_error()) {
					for (auto & curr_message : j) {
						messages[snowflake_not_null(&curr_message, "id")] = message(this).fill_from_json(&curr_message);
					}
				}
				callback(confirmation_callback_t("message_map", messages, http));
			}
		});
	};
	if (limit <= 100) {
		impl(channel_id, around, before, after, limit, callback);
	} else {
		std::function<void(snowflake)> f = [f, callback, impl, channel_id, around, before, limit](snowflake after) {
			impl(channel_id, around, before, after, limit, [&](confirmation_callback_t cc) {
				if (!cc.is_error()) {
					snowflake last_snowflake = after;
					message_map mm = std::get<message_map>(cc.value);
					for (const auto& m : mm) {
						big_mm.emplace(m.first, m.second);
						last_snowflake = m.second.id;
					}
					if (mm.size() < 100) {
						cc.value = big_mm;
						callback(cc);
					} else {
						f(last_snowflake);
					}
				} else {
					cc.value = big_mm;
					callback(cc);
				}
			});
		};
		f(after);
	}
}


void cluster::message_unpin(snowflake channel_id, snowflake message_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "pins/" + std::to_string(message_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::channel_pins_get(snowflake channel_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "pins", m_get, "", [this, callback](json &j, const http_request_completion_t& http) {
		message_map pins_messages;
		confirmation_callback_t e("confirmation", confirmation(), http);
		if (!e.is_error()) {
			for (auto & curr_message : j) {
				pins_messages[snowflake_not_null(&curr_message, "id")] = message(this).fill_from_json(&curr_message);
			}
		}
		if (callback) {
			callback(confirmation_callback_t("message_map", pins_messages, http));
		}
	});
}

};
