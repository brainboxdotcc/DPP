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
#include <map>
#include <dpp/discord.h>
#include <dpp/cluster.h>
#include <dpp/discordclient.h>
#include <dpp/discordevents.h>
#include <dpp/message.h>
#include <dpp/cache.h>
#include <chrono>
#include <iostream>
#include <dpp/nlohmann/json.hpp>
#include <dpp/fmt/format.h>

namespace dpp {

cluster::cluster(const std::string &_token, uint32_t _intents, uint32_t _shards, uint32_t _cluster_id, uint32_t _maxclusters, bool comp, cache_policy_t policy)
	: token(_token), intents(_intents), numshards(_shards), cluster_id(_cluster_id),
	maxclusters(_maxclusters), last_identify(time(NULL) - 5), compressed(comp), cache_policy(policy), rest_ping(0.0)
{
	rest = new request_queue(this);
#ifdef _WIN32
	// Set up winsock.
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		throw dpp::exception("WSAStartup failure");
	}
#endif
}

cluster::~cluster()
{
	delete rest;
#ifdef _WIN32
	WSACleanup();
#endif
}

confirmation_callback_t::confirmation_callback_t(const std::string &_type, const confirmable_t& _value, const http_request_completion_t& _http)
	: type(_type), value(_value), http_info(_http)
{
	if (type == "confirmation") {
		confirmation newvalue = std::get<confirmation>(_value);
		newvalue.success = (http_info.status < 400);
		value = newvalue;
	}
}

bool confirmation_callback_t::is_error() const {
	if (http_info.status >= 400) {
		/* Invalid JSON or 4xx/5xx response */
		return true;
	}
	try {
		json j = json::parse(this->http_info.body);
		if (j.find("code") != j.end() && j.find("errors") != j.end() && j.find("message") != j.end()) {
			if (j["code"].is_number_unsigned() && j["errors"].is_object() && j["message"].is_string()) {
				return true;
			} else {
				return false;
			}
		}
		return false;
	}
	catch (const std::exception &e) {
		/* JSON parse error indicates the content is not JSON.
		 * This means that its an empty body e.g. 204 response, and not an actual error.
		 */
		return false;
	}
}

error_info confirmation_callback_t::get_error() const {
	if (is_error()) {
		json j = json::parse(this->http_info.body);
		error_info e;

		SetInt32NotNull(&j, "code", e.code);
		SetStringNotNull(&j, "message", e.message);
		json& errors = j["errors"];
		for (auto obj = errors.begin(); obj != errors.end(); ++obj) {

			if (obj->find("0") != obj->end()) {
				/* An array of error messages */
				for (auto index = obj->begin(); index != obj->end(); ++index) {
					for (auto fields = index->begin(); fields != index->end(); ++fields) {
						for (auto errordetails = (*fields)["_errors"].begin(); errordetails != (*fields)["_errors"].end(); ++errordetails) {
							error_detail detail;
							detail.code = (*errordetails)["code"].get<std::string>();
							detail.reason = (*errordetails)["message"].get<std::string>();
							detail.field = fields.key();
							detail.object = obj.key();
							e.errors.push_back(detail);
						}
					}
				}

			} else if (obj->find("_errors") != obj->end()) {
				/* An object of error messages */
				for (auto errordetails = (*obj)["_errors"].begin(); errordetails != (*obj)["_errors"].end(); ++errordetails) {
					error_detail detail;
					detail.code = (*errordetails)["code"].get<std::string>();
					detail.reason = (*errordetails)["message"].get<std::string>();
					detail.object = "";
					detail.field = obj.key();
					e.errors.push_back(detail);
				}
			}
		}

		return e;
	}
	return error_info();
}


void cluster::auto_shard(const confirmation_callback_t &shardinfo) {
	gateway g = std::get<gateway>(shardinfo.value);
	numshards = g.shards;
	log(ll_info, fmt::format("Auto Shard: Bot requires {} shard{}", g.shards, g.shards > 1 ? "s" : ""));
	if (g.shards) {
		if (g.session_start_remaining < g.shards) {
			log(ll_critical, fmt::format("Auto Shard: Discord indicates you cannot start any more sessions! Cluster startup aborted. Try again later."));
		} else {
			log(ll_debug, fmt::format("Auto Shard: {} of {} session starts remaining", g.session_start_remaining, g.session_start_total));
			cluster::start(true);
		}
	} else {
		log(ll_critical, "Auto Shard: Could not auto detect shard count! Cluster startup aborted.");
	}
}

void cluster::log(dpp::loglevel severity, const std::string &msg) const {
	if (dispatch.log) {
		/* Pass to user if theyve hooked the event */
		dpp::log_t logmsg(nullptr, msg);
		logmsg.severity = severity;
		logmsg.message = msg;
		dispatch.log(logmsg);
	}
}

dpp::utility::uptime cluster::uptime()
{
	return dpp::utility::uptime(time(NULL) - start_time);
}

void cluster::start(bool return_after) {
	/* Start up all shards */
	if (numshards == 0) {
		get_gateway_bot(std::bind(&cluster::auto_shard, this, std::placeholders::_1));
		if (!return_after) {
			while (true) {
				std::this_thread::sleep_for(std::chrono::seconds(86400));
			}
		}
	} else {
		start_time = time(NULL);

		log(ll_debug, fmt::format("Starting with {} shards...", numshards));

		for (uint32_t s = 0; s < numshards; ++s) {
			/* Filter out shards that arent part of the current cluster, if the bot is clustered */
			if (s % maxclusters == cluster_id) {
				/* Each discord_client spawns its own thread in its Run() */
				try {
					this->shards[s] = new discord_client(this, s, numshards, token, intents, compressed);
					this->shards[s]->Run();
				}
				catch (const std::exception &e) {
					log(dpp::ll_critical, fmt::format("Could not start shard {}: {}", s, e.what()));
				}
				/* Stagger the shard startups */
				std::this_thread::sleep_for(std::chrono::seconds(5));
			}
		}

		/* Get all active DM channels and map them to user id -> dm id */
		this->current_user_get_dms([this](const dpp::confirmation_callback_t& completion) {
			dpp::channel_map dmchannels = std::get<channel_map>(completion.value);
			for (auto & c : dmchannels) {
				for (auto & u : c.second.recipients) {
					this->set_dm_channel(u, c.second.id);
				}
			}
		});

		log(ll_debug, "Shards started.");

		if (!return_after) {
			while (true) {
				std::this_thread::sleep_for(std::chrono::seconds(86400));
			}
		}
	}
}

snowflake cluster::get_dm_channel(snowflake user_id) {
	std::lock_guard<std::mutex> lock(dm_list_lock);
	auto i = dm_channels.find(user_id);
	if (i != dm_channels.end()) {
		return i->second;
	} else {
		return 0;
	}
}

void cluster::set_dm_channel(snowflake user_id, snowflake channel_id) {
	std::lock_guard<std::mutex> lock(dm_list_lock);
	dm_channels[user_id] = channel_id;
}

void cluster::post_rest(const std::string &endpoint, const std::string &major_parameters, const std::string &parameters, http_method method, const std::string &postdata, json_encode_t callback, const std::string &filename, const std::string &filecontent) {
	/* NOTE: This is not a memory leak! The request_queue will free the http_request once it reaches the end of its lifecycle */
	rest->post_request(new http_request(endpoint + "/" + major_parameters, parameters, [endpoint, callback, this](const http_request_completion_t& rv) {
		json j;
		if (rv.error == h_success && !rv.body.empty()) {
			try {
				j = json::parse(rv.body);
			}
			catch (const std::exception &e) {
				/* TODO: Do something clever to handle malformed JSON */
				log(ll_error, fmt::format("post_rest() to {}: {}", endpoint, e.what()));
				return;
			}
		}
		if (callback) {
			callback(j, rv);
		}
	}, postdata, method, filename, filecontent));
}

gateway::gateway(nlohmann::json* j) {
	url = StringNotNull(j, "url");
	shards = Int32NotNull(j, "shards");
	session_start_total = Int32NotNull(&((*j)["session_start_limit"]), "total");
	session_start_remaining  = Int32NotNull(&((*j)["session_start_limit"]), "remaining");
	session_start_reset_after = Int32NotNull(&((*j)["session_start_limit"]), "reset_after");
	session_start_max_concurrency = Int32NotNull(&((*j)["session_start_limit"]), "max_concurrency");
}

void cluster::set_presence(const dpp::presence &p) {
	std::string string_presence = p.build_json();
	for (auto& s : shards) {
		if (s.second->is_connected()) {
			s.second->QueueMessage(string_presence);
		}
	}
}

discord_client* cluster::get_shard(uint32_t id) {
	auto i = shards.find(id);
	if (i != shards.end()) {
		return i->second;
	} else {
		return nullptr;
	}
}

const shard_list& cluster::get_shards() {
	return shards;
}

void cluster::get_gateway_bot(command_completion_event_t callback) {
	this->post_rest(API_PATH "/gateway", "bot", "", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("gateway", gateway(&j), http));
		}
	});
}

void cluster::direct_message_create(snowflake user_id, const message &m, command_completion_event_t callback) {
	/* Find out if a DM channel already exists */
	message msg = m;
	snowflake dm_channel_id = this->get_dm_channel(user_id);
	if (!dm_channel_id) {
		this->create_dm_channel(user_id, [user_id, this, msg, callback](const dpp::confirmation_callback_t& completion) {
			/* NOTE: We are making copies in here for a REASON. Don't try and optimise out these
			 * copies as if we use references, by the time the the thread completes for the callback
			 * the reference is invalid and we get a crash or heap corruption!
			 */
			message m2 = msg;
			dpp::channel c = std::get<channel>(completion.value);
			m2.channel_id = c.id;
			this->set_dm_channel(user_id, c.id);
			message_create(m2, callback);
		});
	} else {
		msg.channel_id = dm_channel_id;
		message_create(msg, callback);
	}
}

void cluster::interaction_response_create(snowflake interaction_id, const std::string &token, const interaction_response &r, command_completion_event_t callback) {
	this->post_rest(API_PATH "/interactions", std::to_string(interaction_id), url_encode(token) + "/callback", m_post, r.build_json(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	}, r.msg->filename, r.msg->filecontent);
}

void cluster::interaction_response_edit(const std::string &token, const message &m, command_completion_event_t callback) {
	this->post_rest(API_PATH "/webhooks", std::to_string(me.id), url_encode(token) + "/messages/@original", m_patch, m.build_json(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	}, m.filename, m.filecontent);
}


void cluster::global_command_create(slashcommand &s, command_completion_event_t callback) {
	this->post_rest(API_PATH "/applications", std::to_string(s.application_id ? s.application_id : me.id), "commands", m_post, s.build_json(false), [s, callback] (json &j, const http_request_completion_t& http) mutable {
		if (j.contains("id")) {
			s.id = SnowflakeNotNull(&j, "id");
		}

		if (callback) {
			callback(confirmation_callback_t("slashcommand", slashcommand().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_command_create(slashcommand &s, snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/applications", std::to_string(s.application_id ? s.application_id : me.id), "guilds/" + std::to_string(guild_id) + "/commands", m_post, s.build_json(false), [s, this, guild_id, callback] (json &j, const http_request_completion_t& http) mutable {
		if (j.contains("id")) {
			s.id = SnowflakeNotNull(&j, "id");
		}

		if (callback) {
			callback(confirmation_callback_t("slashcommand", slashcommand().fill_from_json(&j), http));
		}

		if (http.status < 300 && s.permissions.size()) {
			guild_command_edit_permissions(s, guild_id);
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
	this->post_rest(API_PATH "/applications", std::to_string(commands[0].application_id ? commands[0].application_id : me.id), "guilds/" + std::to_string(guild_id) + "/commands", m_put, j.dump(), [this, callback] (json &j, const http_request_completion_t& http) mutable {
		slashcommand_map slashcommands;
		for (auto & curr_slashcommand : j) {
			slashcommands[SnowflakeNotNull(&curr_slashcommand, "id")] = slashcommand().fill_from_json(&curr_slashcommand);
		}
		if (callback) {
			callback(confirmation_callback_t("slashcommand_map", slashcommands, http));
		}
	});
}

void cluster::global_bulk_command_create(const std::vector<slashcommand> &commands, command_completion_event_t callback) {
	if (commands.empty()) {
		return;
	}
	json j = json::array();
	for (auto & s : commands) {
		j.push_back(json::parse(s.build_json(false)));
	}
	this->post_rest(API_PATH "/applications", std::to_string(commands[0].application_id ? commands[0].application_id : me.id), "commands", m_put, j.dump(), [this, callback] (json &j, const http_request_completion_t& http) mutable {
		slashcommand_map slashcommands;
		for (auto & curr_slashcommand : j) {
			slashcommands[SnowflakeNotNull(&curr_slashcommand, "id")] = slashcommand().fill_from_json(&curr_slashcommand);
		}
		if (callback) {
			callback(confirmation_callback_t("slashcommand_map", slashcommands, http));
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

void cluster::guild_command_edit(const slashcommand &s, snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/applications", std::to_string(s.application_id ? s.application_id : me.id), "guilds/" + std::to_string(guild_id) + "/commands/" + std::to_string(s.id), m_patch, s.build_json(true), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::guild_command_edit_permissions(const slashcommand &s, snowflake guild_id, command_completion_event_t callback) {
	json j;

	if(s.permissions.size())  {
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

void cluster::global_command_delete(snowflake id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/applications", std::to_string(me.id), "commands/" + std::to_string(id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
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

void cluster::message_create(const message &m, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(m.channel_id), "messages", m_post, m.build_json(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("message", message().fill_from_json(&j), http));
		}
	}, m.filename, m.filecontent);
}

void cluster::message_edit(const message &m, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(m.channel_id), "messages/" + std::to_string(m.id), m_patch, m.build_json(true), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("message", message().fill_from_json(&j), http));
		}
	}, m.filename, m.filecontent);
}

void cluster::guild_sticker_create(sticker &s, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(s.guild_id), "stickers", m_post, s.build_json(false), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("sticker", sticker().fill_from_json(&j), http));
		}
	}, s.filename, s.filecontent);
}

void cluster::guild_sticker_modify(sticker &s, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(s.guild_id), "stickers/" + std::to_string(s.id), m_patch, s.build_json(true), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("sticker", sticker().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_sticker_delete(snowflake sticker_id, snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "stickers/" + std::to_string(sticker_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::nitro_sticker_get(snowflake id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/stickers", std::to_string(id), "", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("sticker", sticker().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_sticker_get(snowflake id, snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "stickers/" + std::to_string(id), m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("sticker", sticker().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_stickers_get(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "stickers", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		sticker_map stickers;
		for (auto & curr_sticker : j) {
			stickers[SnowflakeNotNull(&curr_sticker, "id")] = sticker().fill_from_json(&curr_sticker);
		}
		if (callback) {
			callback(confirmation_callback_t("sticker_map", stickers, http));
		}
	});
}

void cluster::sticker_packs_get(command_completion_event_t callback) {
	this->post_rest(API_PATH "/sticker-packs", "", "", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		sticker_pack_map stickerpacks;
		for (auto & curr_stickerpack : j) {
			stickerpacks[SnowflakeNotNull(&curr_stickerpack, "id")] = sticker_pack().fill_from_json(&curr_stickerpack);
		}
		if (callback) {
			callback(confirmation_callback_t("sticker_pack_map", stickerpacks, http));
		}
	});
}


void cluster::message_crosspost(snowflake message_id, snowflake channel_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "messages/" + std::to_string(message_id) + "/crosspost", m_post, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("message", message().fill_from_json(&j), http));
		}
	});
}

void cluster::message_add_reaction(const struct message &m, const std::string &reaction, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(m.channel_id), "messages/" + std::to_string(m.id) + "/reactions/" + dpp::url_encode(reaction) + "/@me", m_put, "", [callback](json &j, const http_request_completion_t& http) {
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

void cluster::message_delete_all_reactions(const struct message &m, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(m.channel_id), "messages/" + std::to_string(m.id) + "/reactions",  m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::message_delete_reaction_emoji(const struct message &m, const std::string &reaction, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(m.channel_id), "messages/" + std::to_string(m.id) + "/reactions/" + dpp::url_encode(reaction), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}


void cluster::message_delete_reaction(const struct message &m, snowflake user_id, const std::string &reaction, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(m.channel_id), "messages/" + std::to_string(m.id) + "/reactions/" + dpp::url_encode(reaction) + "/" + std::to_string(user_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
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
			for (auto & curr_user : j) {
				users[SnowflakeNotNull(&curr_user, "id")] = user().fill_from_json(&curr_user);
			}
			callback(confirmation_callback_t("user_map", users, http));
		}
	});
}

void cluster::message_add_reaction(snowflake message_id, snowflake channel_id, const std::string &reaction, command_completion_event_t callback) {
	message m(channel_id, "");
	m.id = message_id;
	message_add_reaction(m, reaction, callback);
}

void cluster::message_delete_own_reaction(snowflake message_id, snowflake channel_id, const std::string &reaction, command_completion_event_t callback) {
	message m(channel_id, "");
	m.id = message_id;
	message_delete_own_reaction(m, reaction, callback);
}

void cluster::message_delete_reaction(snowflake message_id, snowflake channel_id, snowflake user_id, const std::string &reaction, command_completion_event_t callback) {
	message m(channel_id, "");
	m.id = message_id;
	message_delete_reaction(m, user_id, reaction, callback);
}

void cluster::message_get_reactions(snowflake message_id, snowflake channel_id, const std::string &reaction, snowflake before, snowflake after, snowflake limit, command_completion_event_t callback) {
	message m(channel_id, "");
	m.id = message_id;
	message_get_reactions(m, reaction, before, after, limit, callback);
}

void cluster::message_delete_all_reactions(snowflake message_id, snowflake channel_id, command_completion_event_t callback) {
	message m(channel_id, "");
	m.id = message_id;
	message_delete_all_reactions(m, callback);
}

void cluster::message_delete_reaction_emoji(snowflake message_id, snowflake channel_id, const std::string &reaction, command_completion_event_t callback) {
	message m(channel_id, "");
	m.id = message_id;
	message_delete_reaction_emoji(m, reaction, callback);
}

void cluster::message_get(snowflake message_id, snowflake channel_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "messages/" + std::to_string(message_id), m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("message", message().fill_from_json(&j), http));
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

void cluster::message_delete_bulk(const std::vector<snowflake>& message_ids, snowflake channel_id, command_completion_event_t callback) {
	json j;
	for (auto & m : message_ids) {
		j.push_back(std::to_string(m));
	}
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "messages/bulk-delete", m_delete, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::channel_create(const class channel &c, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(c.guild_id), "channels", m_post, c.build_json(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("channel", channel().fill_from_json(&j), http));
		}
	});
}

void cluster::channel_edit(const class channel &c, command_completion_event_t callback) {
	json j = c.build_json(true);
	auto p = j.find("position");
	if (p != j.end()) {
		j.erase(p);
	}
	this->post_rest(API_PATH "/channels", std::to_string(c.id), "", m_patch, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("channel", channel().fill_from_json(&j), http));
		}
	});
}

void cluster::channel_get(snowflake c, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(c), "", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("channel", channel().fill_from_json(&j), http));
		}
	});
}

void cluster::channel_typing(const class channel &c, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(c.id), "typing", m_post, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::message_pin(snowflake channel_id, snowflake message_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "pins/" + std::to_string(message_id), m_put, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::message_unpin(snowflake channel_id, snowflake message_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "pins/" + std::to_string(message_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::channel_edit_position(const class channel &c, command_completion_event_t callback) {
	json j({ {"id", c.id}, {"position", c.position}  });
	this->post_rest(API_PATH "/guilds", std::to_string(c.guild_id), "channels/" + std::to_string(c.id), m_patch, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("channel", channel().fill_from_json(&j), http));
		}
	});
}

void cluster::channel_edit_permissions(const class channel &c, snowflake overwrite_id, uint32_t allow, uint32_t deny, bool member, command_completion_event_t callback) {
	json j({ {"allow", std::to_string(allow)}, {"deny", std::to_string(deny)}, {"type", member ? 1 : 0}  });
	this->post_rest(API_PATH "/channels", std::to_string(c.id), "permissions/" + std::to_string(overwrite_id), m_put, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::channel_follow_news(const class channel &c, snowflake target_channel_id, command_completion_event_t callback) {
	json j({ {"webhook_channel_id", target_channel_id} });
	this->post_rest(API_PATH "/channels", std::to_string(c.id), "followers", m_post, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::channel_delete_permission(const class channel &c, snowflake overwrite_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(c.id), "permissions/" + std::to_string(overwrite_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::invite_get(const std::string &invitecode, command_completion_event_t callback) {
	this->post_rest(API_PATH "/invites", dpp::url_encode(invitecode), "?with_counts=true", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("invite", invite().fill_from_json(&j), http));
		}
	});
}

void cluster::channel_invites_get(const class channel &c, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(c.id), "invites", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		invite_map invites;
		for (auto & curr_invite : j) {
			invites[StringNotNull(&curr_invite, "code")] = invite().fill_from_json(&curr_invite);
		}
		if (callback) {
			callback(confirmation_callback_t("invite_map", invites, http));
		}
	});
}

void cluster::guild_commands_get(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/applications", std::to_string(me.id), "/guilds/" + std::to_string(guild_id) + "/commands", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		slashcommand_map slashcommands;
		for (auto & curr_slashcommand : j) {
			slashcommands[SnowflakeNotNull(&curr_slashcommand, "id")] = slashcommand().fill_from_json(&curr_slashcommand);
		}
		if (callback) {
			callback(confirmation_callback_t("slashcommand_map", slashcommands, http));
		}
	});
}

void cluster::global_commands_get(command_completion_event_t callback) {
	this->post_rest(API_PATH "/applications", std::to_string(me.id), "commands", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		slashcommand_map slashcommands;
		for (auto & curr_slashcommand : j) {
			slashcommands[SnowflakeNotNull(&curr_slashcommand, "id")] = slashcommand().fill_from_json(&curr_slashcommand);
		}
		if (callback) {
			callback(confirmation_callback_t("slashcommand_map", slashcommands, http));
		}
	});
}


void cluster::get_guild_invites(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "invites", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		invite_map invites;
		for (auto & curr_invite : j) {
			invites[StringNotNull(&curr_invite, "code")] = invite().fill_from_json(&curr_invite);
		}
		if (callback) {
			callback(confirmation_callback_t("invite_map", invites, http));
		}
	});
}

void cluster::guild_get_integrations(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "integrations", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		integration_map integrations;
		for (auto & curr_integration : j) {
			integrations[SnowflakeNotNull(&curr_integration, "id")] = integration().fill_from_json(&curr_integration);
		}
		if (callback) {
			callback(confirmation_callback_t("integration_map", integrations, http));
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

void cluster::guild_get_vanity(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "vanity-url", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("invite", invite().fill_from_json(&j), http));
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

void cluster::guild_modify_integration(snowflake guild_id, const class integration &i, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "integrations/" + std::to_string(i.id), m_patch, i.build_json(), [callback](json &j, const http_request_completion_t& http) {
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

void cluster::guild_sync_integration(snowflake guild_id, snowflake integration_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "integrations/" + std::to_string(integration_id), m_post, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}



void cluster::channel_invite_create(const class channel &c, const class invite &i, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(c.id), "invites", m_post, i.build_json(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("invite", invite().fill_from_json(&j), http));
		}
	});
}

void cluster::pins_get(snowflake channel_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "pins", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		message_map pins_messages;
		for (auto & curr_message : j) {
			pins_messages[SnowflakeNotNull(&curr_message, "id")] = message().fill_from_json(&curr_message);
		}
		if (callback) {
		callback(confirmation_callback_t("message_map", pins_messages, http));
		}
	});
}

void cluster::gdm_add(snowflake channel_id, snowflake user_id, const std::string &access_token, const std::string &nick, command_completion_event_t callback) {
	json params;
	params["access_token"] = access_token;
	params["nick"] = nick;
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "recipients/" + std::to_string(user_id), m_put, params.dump(), [callback](json &j, const http_request_completion_t& http) {
	if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::gdm_remove(snowflake channel_id, snowflake user_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "recipients/" + std::to_string(user_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::invite_delete(const std::string &invitecode, command_completion_event_t callback) {
	this->post_rest(API_PATH "/invites", dpp::url_encode(invitecode), "", m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("invite", invite().fill_from_json(&j), http));
		}
	});
}

void cluster::channel_delete(snowflake channel_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "", m_delete, "", [callback](json &j, const http_request_completion_t& http) {
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

void cluster::guild_edit(const class guild &g, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(g.id), "", m_patch, g.build_json(true), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("guild", guild().fill_from_json(nullptr, &j), http));
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

void cluster::guild_get_preview(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "preview", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("guild", guild().fill_from_json(nullptr, &j), http));
		}
	});
}

void cluster::guild_get_member(snowflake guild_id, snowflake user_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "member/" + std::to_string(user_id), m_get, "", [callback, guild_id, user_id](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("guild_member", guild_member().fill_from_json(&j, guild_id, user_id), http));
		}
	});
}

void cluster::guild_add_member(const guild_member& gm, const std::string &access_token, command_completion_event_t callback) {
	json j;
	try {
		j = json::parse(gm.build_json());
	}
	catch (const std::exception &e) {
		log(ll_error, fmt::format("guild_add_member(): {}", e.what()));
		return;
	}
	j["access_token"] = access_token;
	this->post_rest(API_PATH "/guilds", std::to_string(gm.guild_id), "members/" + std::to_string(gm.user_id), m_put, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::guild_edit_member(const guild_member& gm, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(gm.guild_id), "members/" + std::to_string(gm.user_id), m_patch, gm.build_json(), [&gm, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("guild_member", guild_member().fill_from_json(&j, gm.guild_id, gm.user_id), http));
		}
	});
}

void cluster::guild_member_move(const snowflake channel_id, const snowflake guild_id, const snowflake user_id, command_completion_event_t callback) {
    json j;
    j["channel_id"] = channel_id;

    this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "members/" + std::to_string(user_id), m_patch, j.dump(), [guild_id, user_id, callback](json &j, const http_request_completion_t& http) {
        if (callback) {
            callback(confirmation_callback_t("guild_member", guild_member().fill_from_json(&j, guild_id, user_id), http));
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

void cluster::guild_member_add_role(snowflake guild_id, snowflake user_id, snowflake role_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "members/" + std::to_string(user_id) + "/roles/" + std::to_string(role_id), m_put, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::guild_member_delete_role(snowflake guild_id, snowflake user_id, snowflake role_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "members/" + std::to_string(user_id) + "/roles/" + std::to_string(role_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::guild_member_delete(snowflake guild_id, snowflake user_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "members/" + std::to_string(user_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
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



void cluster::guild_get_members(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "members", m_get, "", [callback, guild_id](json &j, const http_request_completion_t& http) {
		guild_member_map guild_members;
		for (auto & curr_member : j) {
			guild_member gm;
			snowflake user_id = 0;
			if (curr_member.find("user") != curr_member.end()) {
				user_id = SnowflakeNotNull(&(curr_member["user"]), "id");
			}
			guild_members[SnowflakeNotNull(&curr_member, "id")] = guild_member().fill_from_json(&curr_member, guild_id, user_id);
		}
		if (callback) {
				callback(confirmation_callback_t("guild_member_map", guild_members, http));
		}
	});
}


void cluster::template_get(const std::string &code, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", "templates", code, m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("dtemplate", dtemplate().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_create_from_template(const std::string &code, const std::string &name, command_completion_event_t callback) {
	json params;
	params["name"] = name;
	this->post_rest(API_PATH "/guilds", "templates", code, m_post, params.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("guild", guild().fill_from_json(nullptr, &j), http));
		}
	});
}

void cluster::guild_templates_get(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "templates", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		dtemplate_map dtemplates;
		for (auto & curr_dtemplate : j) {
			dtemplates[SnowflakeNotNull(&curr_dtemplate, "id")] = dtemplate().fill_from_json(&curr_dtemplate);
		}
		if (callback) {
				callback(confirmation_callback_t("dtemplate_map", dtemplates, http));
		}
	});
}

void cluster::guild_template_create(snowflake guild_id, const std::string &name, const std::string &description, command_completion_event_t callback) {
	json params;
	params["name"] = name;
	params["description"] = description;
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "templates", m_post, params.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("dtemplate", dtemplate().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_template_sync(snowflake guild_id, const std::string &code, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "templates/" + code, m_put, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("dtemplate", dtemplate().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_template_modify(snowflake guild_id, const std::string &code, const std::string &name, const std::string &description, command_completion_event_t callback) {
	json params;
	params["name"] = name;
	params["description"] = description;
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "templates/" + code, m_patch, params.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("dtemplate", dtemplate().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_template_delete(snowflake guild_id, const std::string &code, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "templates/" + code, m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("dtemplate", dtemplate().fill_from_json(&j), http));
		}
	});
}

void cluster::user_get(snowflake user_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/users", std::to_string(user_id), "", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("user", user().fill_from_json(&j), http));
		}
	});
}

void cluster::current_user_get(command_completion_event_t callback) {
	this->post_rest(API_PATH "/users", "@me", "", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("user", user().fill_from_json(&j), http));
		}
	});
}

void cluster::current_user_get_guilds(command_completion_event_t callback) {
	this->post_rest(API_PATH "/users", "@me", "guilds", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			guild_map guilds;
			for (auto & curr_guild : j) {
				guilds[SnowflakeNotNull(&curr_guild, "id")] = guild().fill_from_json(nullptr, &curr_guild);
			}
			callback(confirmation_callback_t("guild_map", guilds, http));
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

void cluster::role_create(const class role &r, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(r.guild_id), "roles", m_post, r.build_json(), [r, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("role", role().fill_from_json(r.guild_id, &j), http));
		}
	});
}

void cluster::role_edit(const class role &r, command_completion_event_t callback) {
	json j = r.build_json(true);
	auto p = j.find("position");
	if (p != j.end()) {
		j.erase(p);
	}
	this->post_rest(API_PATH "/guilds", std::to_string(r.guild_id), "roles/" + std::to_string(r.id) , m_patch, j.dump(), [r, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("role", role().fill_from_json(r.guild_id, &j), http));
		}
	});
}

void cluster::guild_get_bans(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "bans", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		ban_map bans;
		for (auto & curr_ban : j) {
			bans[SnowflakeNotNull(&curr_ban, "user_id")] = ban().fill_from_json(&curr_ban);
		}
		if (callback) {
			callback(confirmation_callback_t("ban_map", bans, http));
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

void cluster::guild_emojis_get(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "emojis", m_get, "", [guild_id, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			emoji_map emojis;
			for (auto & curr_emoji : j) {
				emojis[SnowflakeNotNull(&curr_emoji, "id")] = emoji().fill_from_json(&curr_emoji);
			}
			callback(confirmation_callback_t("emoji_map", emojis, http));
		}
	});
}

void cluster::guild_emoji_get(snowflake guild_id, snowflake emoji_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "emojis/" + std::to_string(emoji_id), m_get, "", [guild_id, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("emoji", emoji().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_emoji_create(snowflake guild_id, const class emoji& newemoji, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "emojis", m_post, newemoji.build_json(), [guild_id, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("emoji", emoji().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_emoji_edit(snowflake guild_id, const class emoji& newemoji, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "emojis/" + std::to_string(newemoji.id), m_patch, newemoji.build_json(), [guild_id, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("emoji", emoji().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_emoji_delete(snowflake guild_id, snowflake emoji_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "emojis/" + std::to_string(emoji_id), m_delete, "", [guild_id, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::guild_get_prune_counts(snowflake guild_id, const struct prune& pruneinfo, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "prune", m_get, pruneinfo.build_json(false), [guild_id, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("prune", prune().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_begin_prune(snowflake guild_id, const struct prune& pruneinfo, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "prune", m_get, pruneinfo.build_json(true), [guild_id, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("prune", prune().fill_from_json(&j), http));
		}
	});
}

void cluster::guild_get_voice_regions(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "regions", m_get, "", [guild_id, callback](json &j, const http_request_completion_t& http) {
		voiceregion_map voiceregions;
		for (auto & curr_region : j) {
			voiceregions[StringNotNull(&curr_region, "id")] = voiceregion().fill_from_json(&j);
		}
		callback(confirmation_callback_t("voiceregion_map", voiceregions, http));
	});
}

void cluster::get_voice_regions(command_completion_event_t callback) {
	this->post_rest("/voice/v9/regions", "", "", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		voiceregion_map voiceregions;
		for (auto & curr_region : j) {
			voiceregions[StringNotNull(&curr_region, "id")] = voiceregion().fill_from_json(&j);
		}
		callback(confirmation_callback_t("voiceregion_map", voiceregions, http));
	});
}


void cluster::roles_get(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "roles", m_get, "", [guild_id, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			role_map roles;
			for (auto & curr_role : j) {
				roles[SnowflakeNotNull(&curr_role, "id")] = role().fill_from_json(guild_id, &curr_role);
			}
			callback(confirmation_callback_t("role_map", roles, http));
		}
	});
}

void cluster::channels_get(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "channels", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			channel_map channels;
			for (auto & curr_channel: j) {
				channels[SnowflakeNotNull(&curr_channel, "id")] = channel().fill_from_json(&curr_channel);
			}
			callback(confirmation_callback_t("channel_map", channels, http));
		}
	});
}

void cluster::messages_get(snowflake channel_id, snowflake around, snowflake before, snowflake after, snowflake limit, command_completion_event_t callback) {
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
		parameters.append("&limit=" + std::to_string(limit));
	}
	if (!parameters.empty()) {
		parameters[0] = '?';
	}
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "messages" + parameters, m_get, json(), [channel_id, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			message_map messages;
			for (auto & curr_message : j) {
				messages[SnowflakeNotNull(&curr_message, "id")] = message().fill_from_json(&curr_message);
			}
			callback(confirmation_callback_t("message_map", messages, http));
		}
	});
}

void cluster::role_edit_position(const class role &r, command_completion_event_t callback) {
	json j({ {"id", r.id}, {"position", r.position}  });
	this->post_rest(API_PATH "/guilds", std::to_string(r.guild_id), "roles/" + std::to_string(r.id), m_patch, j.dump(), [r, callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("role", role().fill_from_json(r.guild_id, &j), http));
		}
	});
}

void cluster::role_delete(snowflake guild_id, snowflake role_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "roles/" + std::to_string(role_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::current_user_edit(const std::string &nickname, const std::string& image_blob, const image_type type, command_completion_event_t callback) {
	json j = json::parse("{\"nickname\": null}");
	if (!nickname.empty()) {
		j["nickname"] = nickname;
	}
	if (!image_blob.empty()) {
		static const std::map<image_type, std::string> mimetypes = {
			{ i_gif, "image/gif" },
			{ i_jpg, "image/jpeg" },
			{ i_png, "image/png" }
		};
		if (image_blob.size() > MAX_EMOJI_SIZE) {
			throw dpp::exception("User icon file exceeds discord limit of 256 kilobytes");
		}
		j["avatar"] = "data:" + mimetypes.find(type)->second + ";base64," + base64_encode((unsigned char const*)image_blob.data(), image_blob.length());
	}
	this->post_rest(API_PATH "/users", "@me", "", m_patch, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("user", user().fill_from_json(&j), http));
		}
	});
}

void cluster::current_user_leave_guild(snowflake guild_id, command_completion_event_t callback) {
	 this->post_rest(API_PATH "/users", "@me", "guilds/" + std::to_string(guild_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	 });
}

void cluster::thread_create(const std::string& thread_name, snowflake channel_id, uint16_t auto_archive_duration, channel_type thread_type, command_completion_event_t callback)
{
	json j;
	j["name"] = thread_name;
	j["auto_archive_duration"] = auto_archive_duration;
	j["type"] = thread_type;
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "threads", m_post, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("channel", channel().fill_from_json(&j), http));
		}
	});
}
void cluster::thread_create_with_message(const std::string& thread_name, snowflake channel_id, snowflake message_id, uint16_t auto_archive_duration, command_completion_event_t callback)
{
	json j;
	j["name"] = thread_name;
	j["auto_archive_duration"] = auto_archive_duration;
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "messages/" + std::to_string(message_id) + "/threads", m_post, j.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("channel", channel().fill_from_json(&j), http));
		}
	});
}

void cluster::current_user_join_thread(snowflake thread_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(thread_id), "/thread-members/@me", m_put, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::current_user_leave_thread(snowflake thread_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(thread_id), "/thread-members/@me", m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::thread_member_add(snowflake thread_id, snowflake user_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(thread_id), "/thread-members/" + std::to_string(user_id), m_put, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::thread_member_remove(snowflake thread_id, snowflake user_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(thread_id), "/thread-members/" + std::to_string(user_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::get_thread_members(snowflake thread_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(thread_id), "/threads-members", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			thread_member_map thread_members;
			for (auto& curr_member : j) {
				thread_members[SnowflakeNotNull(&curr_member, "user_id")] = thread_member().fill_from_json(&curr_member);
			}
			callback(confirmation_callback_t("thread_member_map", thread_members, http));
		}
	});
}

void cluster::get_active_threads(snowflake channel_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "/threads/active", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			channel_map threads;
			for (auto &curr_thread : j) {
				threads[SnowflakeNotNull(&curr_thread, "id")] = channel().fill_from_json(&curr_thread);
			}
			callback(confirmation_callback_t("channel_map", threads, http));
		}
	});
}

void cluster::get_public_archived_threads(snowflake channel_id, time_t before_timestamp, uint16_t limit, command_completion_event_t callback) {
	std::string parameters;
	if (before_timestamp) {
		parameters.append("&before=" + std::to_string(before_timestamp));
	}
	if (limit) {
		parameters.append("&limit=" + std::to_string(limit));
	}
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "/threads/archived/public" + parameters, m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			channel_map threads;
			for (auto &curr_thread : j) {
				threads[SnowflakeNotNull(&curr_thread, "id")] = channel().fill_from_json(&curr_thread);
			}
			callback(confirmation_callback_t("channel_map", threads, http));
		}
		});
}

void cluster::get_private_archived_threads(snowflake channel_id, time_t before_timestamp, uint16_t limit, command_completion_event_t callback) {
	std::string parameters;
	if (before_timestamp) {
		parameters.append("&before=" + std::to_string(before_timestamp));
	}
	if (limit) {
		parameters.append("&limit=" + std::to_string(limit));
	}
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "/threads/archived/private" + parameters, m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			channel_map threads;
			for (auto &curr_thread : j) {
				threads[SnowflakeNotNull(&curr_thread, "id")] = channel().fill_from_json(&curr_thread);
			}
			callback(confirmation_callback_t("channel_map", threads, http));
		}
	});
}

void cluster::get_joined_private_archived_threads(snowflake channel_id, snowflake before_id, uint16_t limit, command_completion_event_t callback) {
	std::string parameters;
	if (before_id) {
		parameters.append("&before=" + std::to_string(before_id));
	}
	if (limit) {
		parameters.append("&limit=" + std::to_string(limit));
	}
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "/users/@me/threads/archived/private" + parameters, m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			channel_map threads;
			for (auto &curr_thread : j) {
				threads[SnowflakeNotNull(&curr_thread, "id")] = channel().fill_from_json(&curr_thread);
			}
			callback(confirmation_callback_t("channel_map", threads, http));
		}
	});
}
void cluster::current_user_get_dms(command_completion_event_t callback) {
	this->post_rest(API_PATH "/users", "@me", "channels", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			channel_map channels;
			for (auto & curr_channel: j) {
				channels[SnowflakeNotNull(&curr_channel, "id")] = channel().fill_from_json(&curr_channel);
			}
			callback(confirmation_callback_t("channel_map", channels, http));
		}
	});
}

void cluster::create_dm_channel(snowflake user_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/users", "@me", "channels", m_post, json({{"recipient_id", std::to_string(user_id)}}).dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("channel", channel().fill_from_json(&j), http));
		}
	});
}

void cluster::create_webhook(const class webhook &w, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(w.channel_id), "webhooks", m_post, w.build_json(false), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("webhook", webhook().fill_from_json(&j), http));
		}
	});
}

void cluster::get_guild_webhooks(snowflake guild_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/guilds", std::to_string(guild_id), "webhooks", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			webhook_map webhooks;
			for (auto & curr_webhook: j) {
				webhooks[SnowflakeNotNull(&curr_webhook, "id")] = webhook().fill_from_json(&curr_webhook);
			}
			callback(confirmation_callback_t("webhook_map", webhooks, http));
		}
	});
}

void cluster::get_channel_webhooks(snowflake channel_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/channels", std::to_string(channel_id), "webhooks", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			webhook_map webhooks;
			for (auto & curr_webhook: j) {
				webhooks[SnowflakeNotNull(&curr_webhook, "id")] = webhook().fill_from_json(&curr_webhook);
			}
			callback(confirmation_callback_t("webhook_map", webhooks, http));
		}
	});
}

void cluster::get_webhook(snowflake webhook_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/webhooks", std::to_string(webhook_id), "", m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("webhook", webhook().fill_from_json(&j), http));
		}
	});
}

void cluster::get_webhook_with_token(snowflake webhook_id, const std::string &token, command_completion_event_t callback) {
	this->post_rest(API_PATH "/webhooks", std::to_string(webhook_id), dpp::url_encode(token), m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("webhook", webhook().fill_from_json(&j), http));
		}
	});
}

void cluster::edit_webhook(const class webhook& wh, command_completion_event_t callback) {
	this->post_rest(API_PATH "/webhooks", std::to_string(wh.id), "", m_patch, wh.build_json(true), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("webhook", webhook().fill_from_json(&j), http));
		}
	});
}

void cluster::edit_webhook_with_token(const class webhook& wh, command_completion_event_t callback) {
	json jwh;
	try {
		jwh = json::parse(wh.build_json(true));
	}
	catch (const std::exception &e) {
		log(ll_error, fmt::format("edit_webhook_with_token(): {}", e.what()));
		return;
	}
	if (jwh.find("channel_id") != jwh.end()) {
		jwh.erase(jwh.find("channel_id"));
	}
	this->post_rest(API_PATH "/webhooks", std::to_string(wh.id), dpp::url_encode(wh.token), m_patch, jwh.dump(), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("webhook", webhook().fill_from_json(&j), http));
		}
	});
}

void cluster::delete_webhook(snowflake webhook_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/webhooks", std::to_string(webhook_id), "", m_delete, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::delete_webhook_with_token(snowflake webhook_id, const std::string &token, command_completion_event_t callback) {
	this->post_rest(API_PATH "/webhooks", std::to_string(webhook_id), dpp::url_encode(token), m_get, "", [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("confirmation", confirmation(), http));
		}
	});
}

void cluster::execute_webhook(const class webhook &wh, const struct message& m, bool wait, snowflake thread_id, command_completion_event_t callback) {
	std::string parameters;
	if (wait) {
		parameters.append("&wait=true");
	}
	if (thread_id) {
		parameters.append("&thread_id=" + std::to_string(thread_id));
	}
	this->post_rest(API_PATH "/webhooks", std::to_string(wh.id), dpp::url_encode(!wh.token.empty() ? wh.token: token), m_post, m.build_json(false), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("message", message().fill_from_json(&j), http));
		}
	});
}

void cluster::get_webhook_message(const class webhook &wh, command_completion_event_t callback)
{
	this->post_rest(API_PATH "/webhooks", std::to_string(wh.id), dpp::url_encode(!wh.token.empty() ? wh.token: token) + "/messages/@original", m_get, "", [callback](json &j, const http_request_completion_t &http){
		if (callback){
			callback(confirmation_callback_t("message", message().fill_from_json(&j), http));
		}
	});
}

void cluster::edit_webhook_message(const class webhook &wh, const struct message& m, command_completion_event_t callback) {
	this->post_rest(API_PATH "/webhooks", std::to_string(wh.id), dpp::url_encode(!wh.token.empty() ? wh.token: token) + "/messages/" + std::to_string(m.id), m_patch, m.build_json(false), [callback](json &j, const http_request_completion_t& http) {
		if (callback) {
			callback(confirmation_callback_t("message", message().fill_from_json(&j), http));
		}
	});
}

void cluster::delete_webhook_message(const class webhook &wh, snowflake message_id, command_completion_event_t callback) {
	this->post_rest(API_PATH "/webhooks", std::to_string(wh.id), dpp::url_encode(!wh.token.empty() ? wh.token: token) + "/messages/" + std::to_string(message_id), m_delete, "", [callback](json &j, const http_request_completion_t& http) {
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

void cluster::on_log (std::function<void(const log_t& _event)> _log) {
	this->dispatch.log = _log;
}

void cluster::on_voice_state_update (std::function<void(const voice_state_update_t& _event)> _voice_state_update) {
	this->dispatch.voice_state_update = _voice_state_update;
}

void cluster::on_stage_instance_create (std::function<void(const stage_instance_create_t& _event)> _stage_instance_create) {
	this->dispatch.stage_instance_create = _stage_instance_create;
}

void cluster::on_stage_instance_delete (std::function<void(const stage_instance_delete_t& _event)> _stage_instance_delete) {
	this->dispatch.stage_instance_delete = _stage_instance_delete;
}

void cluster::on_interaction_create (std::function<void(const interaction_create_t& _event)> _interaction_create) {
	this->dispatch.interaction_create = _interaction_create;
}

void cluster::on_button_click (std::function<void(const button_click_t& _event)> _button_click) {
	this->dispatch.button_click = _button_click;
}

void cluster::on_select_click (std::function<void(const select_click_t& _event)> _select_click) {
	this->dispatch.select_click = _select_click;
}

void cluster::on_guild_delete (std::function<void(const guild_delete_t& _event)> _guild_delete) {
	this->dispatch.guild_delete = _guild_delete;
}

void cluster::on_channel_delete (std::function<void(const channel_delete_t& _event)> _channel_delete) {
	this->dispatch.channel_delete = _channel_delete;
}

void cluster::on_channel_update (std::function<void(const channel_update_t& _event)> _channel_update) {
	this->dispatch.channel_update = _channel_update;
}

void cluster::on_ready (std::function<void(const ready_t& _event)> _ready) {
	this->dispatch.ready = _ready;
}

void cluster::on_message_delete (std::function<void(const message_delete_t& _event)> _message_delete) {
	this->dispatch.message_delete = _message_delete;
}

void cluster::on_application_command_delete (std::function<void(const application_command_delete_t& _event)> _application_command_delete) {
	this->dispatch.application_command_delete = _application_command_delete;
}

void cluster::on_guild_member_remove (std::function<void(const guild_member_remove_t& _event)> _guild_member_remove) {
	this->dispatch.guild_member_remove = _guild_member_remove;
}

void cluster::on_application_command_create (std::function<void(const application_command_create_t& _event)> _application_command_create) {
	this->dispatch.application_command_create = _application_command_create;
}

void cluster::on_resumed (std::function<void(const resumed_t& _event)> _resumed) {
	this->dispatch.resumed = _resumed;
}

void cluster::on_guild_role_create (std::function<void(const guild_role_create_t& _event)> _guild_role_create) {
	this->dispatch.guild_role_create = _guild_role_create;
}

void cluster::on_typing_start (std::function<void(const typing_start_t& _event)> _typing_start) {
	this->dispatch.typing_start = _typing_start;
}

void cluster::on_message_reaction_add (std::function<void(const message_reaction_add_t& _event)> _message_reaction_add) {
	this->dispatch.message_reaction_add = _message_reaction_add;
}

void cluster::on_guild_members_chunk (std::function<void(const guild_members_chunk_t& _event)> _guild_members_chunk) {
	this->dispatch.guild_members_chunk = _guild_members_chunk;
}

void cluster::on_message_reaction_remove (std::function<void(const message_reaction_remove_t& _event)> _message_reaction_remove) {
	this->dispatch.message_reaction_remove = _message_reaction_remove;
}

void cluster::on_guild_create (std::function<void(const guild_create_t& _event)> _guild_create) {
	this->dispatch.guild_create = _guild_create;
}

void cluster::on_channel_create (std::function<void(const channel_create_t& _event)> _channel_create) {
	this->dispatch.channel_create = _channel_create;
}

void cluster::on_message_reaction_remove_emoji (std::function<void(const message_reaction_remove_emoji_t& _event)> _message_reaction_remove_emoji) {
	this->dispatch.message_reaction_remove_emoji = _message_reaction_remove_emoji;
}

void cluster::on_message_delete_bulk (std::function<void(const message_delete_bulk_t& _event)> _message_delete_bulk) {
	this->dispatch.message_delete_bulk = _message_delete_bulk;
}

void cluster::on_guild_role_update (std::function<void(const guild_role_update_t& _event)> _guild_role_update) {
	this->dispatch.guild_role_update = _guild_role_update;
}

void cluster::on_guild_role_delete (std::function<void(const guild_role_delete_t& _event)> _guild_role_delete) {
	this->dispatch.guild_role_delete = _guild_role_delete;
}

void cluster::on_channel_pins_update (std::function<void(const channel_pins_update_t& _event)> _channel_pins_update) {
	this->dispatch.channel_pins_update = _channel_pins_update;
}

void cluster::on_message_reaction_remove_all (std::function<void(const message_reaction_remove_all_t& _event)> _message_reaction_remove_all) {
	this->dispatch.message_reaction_remove_all = _message_reaction_remove_all;
}

void cluster::on_thread_create (std::function<void(const thread_create_t& _event)> _thread_create) {
	this->dispatch.thread_create = _thread_create;
}

void cluster::on_thread_update (std::function<void(const thread_update_t& _event)> _thread_update) {
	this->dispatch.thread_update = _thread_update;
}

void cluster::on_thread_delete (std::function<void(const thread_delete_t& _event)> _thread_delete) {
	this->dispatch.thread_delete = _thread_delete;
}

void cluster::on_thread_list_sync (std::function<void(const thread_list_sync_t& _event)> _thread_list_sync) {
	this->dispatch.thread_list_sync = _thread_list_sync;
}

void cluster::on_thread_member_update (std::function<void(const thread_member_update_t& _event)> _thread_member_update) {
	this->dispatch.thread_member_update = _thread_member_update;
}

void cluster::on_thread_members_update (std::function<void(const thread_members_update_t& _event)> _thread_members_update) {
	this->dispatch.thread_members_update = _thread_members_update;
}

void cluster::on_voice_server_update (std::function<void(const voice_server_update_t& _event)> _voice_server_update) {
	this->dispatch.voice_server_update = _voice_server_update;
}

void cluster::on_guild_emojis_update (std::function<void(const guild_emojis_update_t& _event)> _guild_emojis_update) {
	this->dispatch.guild_emojis_update = _guild_emojis_update;
}

void cluster::on_guild_stickers_update (std::function<void(const guild_stickers_update_t& _event)> _guild_stickers_update) {
	this->dispatch.stickers_update = _guild_stickers_update;
}

void cluster::on_presence_update (std::function<void(const presence_update_t& _event)> _presence_update) {
	this->dispatch.presence_update = _presence_update;
}

void cluster::on_webhooks_update (std::function<void(const webhooks_update_t& _event)> _webhooks_update) {
	this->dispatch.webhooks_update = _webhooks_update;
}

void cluster::on_guild_member_add (std::function<void(const guild_member_add_t& _event)> _guild_member_add) {
	this->dispatch.guild_member_add = _guild_member_add;
}

void cluster::on_invite_delete (std::function<void(const invite_delete_t& _event)> _invite_delete) {
	this->dispatch.invite_delete = _invite_delete;
}

void cluster::on_guild_update (std::function<void(const guild_update_t& _event)> _guild_update) {
	this->dispatch.guild_update = _guild_update;
}

void cluster::on_guild_integrations_update (std::function<void(const guild_integrations_update_t& _event)> _guild_integrations_update) {
	this->dispatch.guild_integrations_update = _guild_integrations_update;
}

void cluster::on_guild_member_update (std::function<void(const guild_member_update_t& _event)> _guild_member_update) {
	this->dispatch.guild_member_update = _guild_member_update;
}

void cluster::on_application_command_update (std::function<void(const application_command_update_t& _event)> _application_command_update) {
	this->dispatch.application_command_update = _application_command_update;
}

void cluster::on_invite_create (std::function<void(const invite_create_t& _event)> _invite_create) {
	this->dispatch.invite_create = _invite_create;
}

void cluster::on_message_update (std::function<void(const message_update_t& _event)> _message_update) {
	this->dispatch.message_update = _message_update;
}

void cluster::on_user_update (std::function<void(const user_update_t& _event)> _user_update) {
	this->dispatch.user_update = _user_update;
}

void cluster::on_message_create (std::function<void(const message_create_t& _event)> _message_create) {
	this->dispatch.message_create = _message_create;
}

void cluster::on_guild_ban_add (std::function<void(const guild_ban_add_t& _event)> _guild_ban_add) {
	this->dispatch.guild_ban_add = _guild_ban_add;
}

void cluster::on_guild_ban_remove (std::function<void(const guild_ban_remove_t& _event)> _guild_ban_remove) {
	this->dispatch.guild_ban_remove = _guild_ban_remove;
}

void cluster::on_integration_create (std::function<void(const integration_create_t& _event)> _integration_create) {
	this->dispatch.integration_create = _integration_create;
}

void cluster::on_integration_update (std::function<void(const integration_update_t& _event)> _integration_update) {
	this->dispatch.integration_update = _integration_update;
}

void cluster::on_integration_delete (std::function<void(const integration_delete_t& _event)> _integration_delete) {
	this->dispatch.integration_delete = _integration_delete;
}

void cluster::on_voice_buffer_send (std::function<void(const voice_buffer_send_t& _event)> _voice_buffer_send) {
	this->dispatch.voice_buffer_send = _voice_buffer_send;
}

void cluster::on_voice_user_talking (std::function<void(const voice_user_talking_t& _event)> _voice_user_talking) {
	this->dispatch.voice_user_talking = _voice_user_talking;
}

void cluster::on_voice_ready (std::function<void(const voice_ready_t& _event)> _voice_ready) {
	this->dispatch.voice_ready = _voice_ready;
}

void cluster::on_voice_receive (std::function<void(const voice_receive_t& _event)> _voice_receive) {
	this->dispatch.voice_receive = _voice_receive;
}

void cluster::on_voice_track_marker (std::function<void(const voice_track_marker_t& _event)> _voice_track_marker) {
	this->dispatch.voice_track_marker = _voice_track_marker;
}

void cluster::on_guild_join_request_delete(std::function<void(const guild_join_request_delete_t& _event)> _guild_join_request_delete) {
	this->dispatch.guild_join_request_delete = _guild_join_request_delete;
}

};
