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

void cluster::guild_current_member_edit(snowflake guild_id, const std::string &nickname, command_completion_event_t callback) {
	std::string o;
	if (nickname.empty()) {
		o = "{\"nick\": null}";
	} else {
		o = json({{"nick", nickname}}).dump();
	}
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "members/@me", m_patch, o, [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::guild_auditlog_get(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "audit-logs", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("auditlog", auditlog().fill_from_json(&j), http));
		}
	});
}


void cluster::guild_ban_add(snowflake guild_id, snowflake user_id, uint32_t delete_message_days, const std::string &reason, command_completion_event_t callback) {
	json j;
	if (delete_message_days > 7)
		delete_message_days = 7;
	if (!reason.empty())
		j["reason"] = reason;
	if (delete_message_days)
		j["delete_message_days"] = delete_message_days;
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "bans/" + std::to_string(user_id), m_put, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::guild_ban_delete(snowflake guild_id, snowflake user_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "bans/" + std::to_string(user_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::guild_create(const class guild &g, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", "", "", m_post, g.build_json(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("guild", guild().fill_from_json(nullptr, &j), http));
		}
	});
}


void cluster::guild_delete(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "", m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::guild_delete_integration(snowflake guild_id, snowflake integration_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "integrations/" + std::to_string(integration_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::guild_edit(const class guild &g, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(g.id), "", m_patch, g.build_json(true), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("guild", guild().fill_from_json(nullptr, &j), http));
		}
	});
}


void cluster::guild_edit_widget(snowflake guild_id, const class guild_widget &gw, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "widget", m_patch, gw.build_json(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("guild_widget", guild_widget().fill_from_json(&j), http));
		}
	});
}


void cluster::guild_get_ban(snowflake guild_id, snowflake user_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "bans/" + std::to_string(user_id), m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("ban", ban().fill_from_json(&j), http));
		}
	});
}


void cluster::guild_get_bans(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "bans", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		confirmation_callback_t e("confirmation", confirmation(), http);
		ban_map bans;
		if (!e.is_error()) {
			for (auto & curr_ban : j) {
				bans[snowflake_not_null(&curr_ban, "user_id")] = ban().fill_from_json(&curr_ban);
			}
		}
		if (callback) {
			callback(confirmation_callback_t("ban_map", bans, http));
		}
	});
}


void cluster::guild_get(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("guild", guild().fill_from_json(nullptr, &j), http));
		}
	});
}


void cluster::guild_get_integrations(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "integrations", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		integration_map integrations;
		confirmation_callback_t e("confirmation", confirmation(), http);
		if (!e.is_error()) {
			for (auto & curr_integration : j) {
				integrations[snowflake_not_null(&curr_integration, "id")] = integration().fill_from_json(&curr_integration);
			}
		}
		if (callback) {
			callback(confirmation_callback_t("integration_map", integrations, http));
		}
	});
}


void cluster::guild_get_preview(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "preview", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("guild", guild().fill_from_json(nullptr, &j), http));
		}
	});
}


void cluster::guild_get_vanity(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "vanity-url", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("invite", invite().fill_from_json(&j), http));
		}
	});
}


void cluster::guild_get_widget(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "widget", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("guild_widget", guild_widget().fill_from_json(&j), http));
		}
	});
}


void cluster::guild_modify_integration(snowflake guild_id, const class integration &i, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "integrations/" + std::to_string(i.id), m_patch, i.build_json(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::guild_get_prune_counts(snowflake guild_id, const struct prune& pruneinfo, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "prune", m_get, pruneinfo.build_json(false), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("prune", prune().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_begin_prune(snowflake guild_id, const struct prune& pruneinfo, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "prune", m_get, pruneinfo.build_json(true), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("prune", prune().fill_from_json(&j), http));
		}
	});
}


void cluster::guild_set_nickname(snowflake guild_id, const std::string &nickname, command_completion_event_t callback) {
	std::string o;
	if (nickname.empty()) {
		o = "{\"nick\": null}";
	} else {
		o = json({{"nick", nickname}}).dump();
	}
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "members/@me/nick", m_patch, o, [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::guild_sync_integration(snowflake guild_id, snowflake integration_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "integrations/" + std::to_string(integration_id), m_post, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


};
