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
#include <dpp/appcommand.h>
#include <dpp/cluster.h>
#include <dpp/nlohmann/json.hpp>

namespace dpp {

void cluster::global_bulk_command_create(const std::vector<slashcommand> &commands, command_completion_event_t callback) {
	if (commands.empty()) {
		return;
	}
	json j = json::array();
	for (auto & s : commands) {
		j.push_back(json::parse(s.build_json(false)));
	}
	this->post_rest(API_PATH "/applications", std::to_string(commands[0].application_id ? commands[0].application_id : me.id), "commands", m_put, j.dump(), [callback] (json &j, const http_request_completion_t& http) mutable {
		confirmation_callback_t e("confirmation", confirmation(), http);
		slashcommand_map slashcommands;
		if (!e.is_error()) {
			for (auto & curr_slashcommand : j) {
				slashcommands[snowflake_not_null(&curr_slashcommand, "id")] = slashcommand().fill_from_json(&curr_slashcommand);
			}
		}
		if (callback) {
			callback(confirmation_callback_t("slashcommand_map", slashcommands, http));
		}
	});
}

void cluster::global_command_create(const slashcommand &s, command_completion_event_t callback) {
	this->post_rest(API_PATH "/applications", std::to_string(s.application_id ? s.application_id : me.id), "commands", m_post, s.build_json(false), [s, callback] (json &j, const http_request_completion_t& http) mutable {
		if (callback) {
			callback(confirmation_callback_t("slashcommand", slashcommand().fill_from_json(&j), http));
		}
	});
}

void cluster::global_command_get(snowflake id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/applications", std::to_string(me.id), "commands/" + std::to_string(id), m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("slashcommand", slashcommand().fill_from_json(&j), http));
		}
	});
}

void cluster::global_command_delete(snowflake id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/applications", std::to_string(me.id), "commands/" + std::to_string(id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::global_command_edit(const slashcommand &s, command_completion_event_t callback) {
	this->post_rest(API_PATH "/applications", std::to_string(s.application_id ? s.application_id : me.id), "commands/" + std::to_string(s.id), m_patch, s.build_json(true), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::global_commands_get(command_completion_event_t callback) {
	this->post_rest(API_PATH "/applications", std::to_string(me.id), "commands", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		slashcommand_map slashcommands;
		confirmation_callback_t e("confirmation", confirmation(), http);
		if (!e.is_error()) {
			for (auto & curr_slashcommand : j) {
				slashcommands[snowflake_not_null(&curr_slashcommand, "id")] = slashcommand().fill_from_json(&curr_slashcommand);
			}
		}
		if (callback) {
			callback(confirmation_callback_t("slashcommand_map", slashcommands, http));
		}
	});
}


void cluster::guild_bulk_command_create(const std::vector<slashcommand> &commands, snowflake guild_id, command_completion_event_t callback) {
	if (commands.empty()) {
		return;
	}
	json j = json::array();
	for (auto & s : commands) {
		j.push_back(json::parse(s.build_json(false)));
	}
	this->post_rest(API_PATH "/applications", std::to_string(commands[0].application_id ? commands[0].application_id : me.id), "guilds/" + std::to_string(guild_id) + "/commands", m_put, j.dump(), [callback] (json &j, const http_request_completion_t& http) mutable {
		slashcommand_map slashcommands;
		confirmation_callback_t e("confirmation", confirmation(), http);
		if (!e.is_error()) {
			for (auto & curr_slashcommand : j) {
				slashcommands[snowflake_not_null(&curr_slashcommand, "id")] = slashcommand().fill_from_json(&curr_slashcommand);
			}
		}
		if (callback) {
			callback(confirmation_callback_t("slashcommand_map", slashcommands, http));
		}
	});
}

void cluster::guild_commands_get_permissions(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/applications", std::to_string(me.id), "guilds/" + std::to_string(guild_id) + "/commands/permissions", m_get, "", [callback](json &j, const http_request_completion_t& http)  {
		guild_command_permissions_map permissions_map;
		confirmation_callback_t e("confirmation", confirmation(), http);
		if (!e.is_error()) {
			for (auto & jperm : j) {
				permissions_map[snowflake_not_null(&jperm, "id")] = guild_command_permissions().fill_from_json(&jperm);
			}
		}
		if (callback) {
			callback(confirmation_callback_t("guild_command_permissions_map", permissions_map, http));
		}
	});
}

void cluster::guild_bulk_command_edit_permissions(const std::vector<slashcommand> &commands, snowflake guild_id, command_completion_event_t callback) {
	if (commands.empty()) {
		return;
	}
	json j = json::array();
	for (auto & s : commands) {
		json jcommand;
		jcommand["id"] = s.id;
		jcommand["permissions"] = json::array();
		for (auto & c : s.permissions) {
			jcommand["permissions"].push_back(c);
		}
		j.push_back(jcommand);
	}
	this->post_rest(API_PATH "/applications", std::to_string(me.id), "guilds/" + std::to_string(guild_id) + "/commands/permissions", m_put, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		guild_command_permissions_map permissions_map;
		confirmation_callback_t e("confirmation", confirmation(), http);
		if (!e.is_error()) {
			for (auto & jperm : j) {
				permissions_map[snowflake_not_null(&jperm, "id")] = guild_command_permissions().fill_from_json(&jperm);
			}
		}
		if (callback) {
			callback(confirmation_callback_t("guild_command_permissions_map", permissions_map, http));
		}
	});
}

void cluster::guild_command_create(const slashcommand &s, snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/applications", std::to_string(s.application_id ? s.application_id : me.id), "guilds/" + std::to_string(guild_id) + "/commands", m_post, s.build_json(false), [s, this, guild_id, callback] (json &j, const http_request_completion_t& http) mutable {
		if (callback) {
			callback(confirmation_callback_t("slashcommand", slashcommand().fill_from_json(&j), http));
		}

		if (http.status < 300 && s.permissions.size()) {
			slashcommand n;
			n.fill_from_json(&j);
			n.permissions = s.permissions;
			guild_command_edit_permissions(n, guild_id);
		}
	});
}

void cluster::guild_command_delete(snowflake id, snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/applications", std::to_string(me.id), "guilds/" + std::to_string(guild_id) + "/commands/" + std::to_string(id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::guild_command_edit_permissions(const slashcommand &s, snowflake guild_id, command_completion_event_t callback) {
	json j;

	if(!s.permissions.empty())  {
		j["permissions"] = json();

		for(const auto& perm : s.permissions) {
			json jperm = perm;
			j["permissions"].push_back(jperm);
		}
	}

	this->post_rest(API_PATH "/applications", std::to_string(s.application_id ? s.application_id : me.id), "guilds/" + std::to_string(guild_id) + "/commands/" + std::to_string(s.id) + "/permissions", m_put, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::guild_command_get(snowflake id, snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/applications", std::to_string(me.id), "guilds/" + std::to_string(guild_id) + "/commands/" + std::to_string(id), m_get, "", [callback](json &j, const http_request_completion_t& http) {
		slashcommand slashcommand;
		confirmation_callback_t e("confirmation", confirmation(), http);
		if (!e.is_error()) {
			slashcommand.fill_from_json(&j);
		}
		if (callback) {
			callback(confirmation_callback_t("slashcommand", slashcommand, http));
		}
	});
}

void cluster::guild_command_get_permissions(snowflake id, snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/applications", std::to_string(me.id), "guilds/" + std::to_string(guild_id) + "/commands/" + std::to_string(id) + "/permissions", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		guild_command_permissions permissions;
		confirmation_callback_t e("confirmation", confirmation(), http);
		if (!e.is_error()) {
			permissions.fill_from_json(&j);
		}
		if (callback) {
			callback(confirmation_callback_t("guild_command_permissions", permissions, http));
		}
	});
}

void cluster::guild_command_edit(const slashcommand &s, snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/applications", std::to_string(s.application_id ? s.application_id : me.id), "guilds/" + std::to_string(guild_id) + "/commands/" + std::to_string(s.id), m_patch, s.build_json(true), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::guild_commands_get(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/applications", std::to_string(me.id), "/guilds/" + std::to_string(guild_id) + "/commands", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		slashcommand_map slashcommands;
		confirmation_callback_t e("confirmation", confirmation(), http);
		if (!e.is_error()) {
			for (auto & curr_slashcommand : j) {
				slashcommands[snowflake_not_null(&curr_slashcommand, "id")] = slashcommand().fill_from_json(&curr_slashcommand);
			}
		}
		if (callback) {
			callback(confirmation_callback_t("slashcommand_map", slashcommands, http));
		}
	});
}

void cluster::interaction_response_create(snowflake interaction_id, const std::string &token, const interaction_response &r, command_completion_event_t callback) {
	this->post_rest_multipart(API_PATH "/interactions", std::to_string(interaction_id), url_encode(token) + "/callback", m_post, r.build_json(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	}, r.msg->filename, r.msg->filecontent);
}

void cluster::interaction_response_edit(const std::string &token, const message &m, command_completion_event_t callback) {
	this->post_rest_multipart(API_PATH "/webhooks", std::to_string(me.id), url_encode(token) + "/messages/@original", m_patch, m.build_json(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	}, m.filename, m.filecontent);
}

};
