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
#include <dpp/exception.h>
#include <dpp/cluster.h>
#include <dpp/discordclient.h>
#include <dpp/discordevents.h>
#include <dpp/message.h>
#include <dpp/cache.h>
#include <chrono>
#include <iostream>
#include <dpp/nlohmann/json.hpp>
#include <utility>
#include <dpp/fmt/format.h>
#include <algorithm>

namespace dpp {

/**
 * @brief An audit reason for each thread. These are per-thread to make the cluster
 * methods like cluster::get_audit_reason and cluster::set_audit_reason thread safe across
 * multiple threads. You must ensure you set the audit reason on the same thread that makes
 * the request associated with it.
 */
thread_local std::string audit_reason;

cluster::cluster(const std::string &_token, uint32_t _intents, uint32_t _shards, uint32_t _cluster_id, uint32_t _maxclusters, bool comp, cache_policy_t policy)
	: rest(nullptr), raw_rest(nullptr), compressed(comp), start_time(0), token(_token), last_identify(time(NULL) - 5), intents(_intents),
	numshards(_shards), cluster_id(_cluster_id), maxclusters(_maxclusters), rest_ping(0.0), cache_policy(policy), ws_mode(ws_json)
{
	rest = new request_queue(this);
	raw_rest = new request_queue(this);
#ifdef _WIN32
	// Set up winsock.
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2, 2), &wsadata)) {
		throw dpp::connection_exception("WSAStartup failure");
	}
#endif
}

cluster::~cluster()
{
	delete rest;
	delete raw_rest;
#ifdef _WIN32
	WSACleanup();
#endif
}

cluster& cluster::set_websocket_protocol(websocket_protocol_t mode) {
	ws_mode = mode;
	return *this;
}




void cluster::auto_shard(const confirmation_callback_t &shardinfo) {
	gateway g = std::get<gateway>(shardinfo.value);
	numshards = g.shards;
	if (g.shards) {
		log(ll_info, fmt::format("Auto Shard: Bot requires {} shard{}", g.shards, (g.shards > 1) ? "s" : ""));
		if (g.session_start_remaining < g.shards) {
			log(ll_critical, fmt::format("Auto Shard: Discord indicates you cannot start any more sessions! Cluster startup aborted. Try again later."));
		} else {
			log(ll_debug, fmt::format("Auto Shard: {} of {} session starts remaining", g.session_start_remaining, g.session_start_total));
			cluster::start(true);
		}
	} else {
		if (shardinfo.is_error()) {
			throw dpp::rest_exception(fmt::format("Auto Shard: Could not get shard count ({} [code: {}]). Cluster startup aborted.", shardinfo.get_error().message, shardinfo.get_error().code));
		} else {
			throw dpp::rest_exception("Auto Shard: Could not get shard count (unknown error, check your connection). Cluster startup aborted.");
		}
	}
}

void cluster::log(dpp::loglevel severity, const std::string &msg) const {
	if (!dispatch.log.empty()) {
		/* Pass to user if theyve hooked the event */
		dpp::log_t logmsg(nullptr, msg);
		logmsg.severity = severity;
		logmsg.message = msg;
		call_event(dispatch.log, logmsg);
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
					this->shards[s] = new discord_client(this, s, numshards, token, intents, compressed, ws_mode);
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
	}, postdata, method, get_audit_reason(), filename, filecontent));
}

void cluster::request(const std::string &url, http_method method, http_completion_event callback, const std::string &postdata, const std::string &mimetype, const std::multimap<std::string, std::string> &headers) {
	/* NOTE: This is not a memory leak! The request_queue will free the http_request once it reaches the end of its lifecycle */
	raw_rest->post_request(new http_request(url, callback, method, postdata, mimetype, headers));
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
	json pres = json::parse(p.build_json());
	for (auto& s : shards) {
		if (s.second->is_connected()) {
			s.second->QueueMessage(s.second->jsonobj_to_string(pres));
		}
	}
}

cluster& cluster::set_audit_reason(const std::string &reason) {
	audit_reason = reason;
	return *this;
}

cluster& cluster::clear_audit_reason() {
	audit_reason.clear();
	return *this;
}

std::string cluster::get_audit_reason() {
	std::string r = audit_reason;
	audit_reason.clear();
	return r;
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


#define detach(container, ptr) { \
	for (auto i = container.begin(); i != container.end(); ++i) { \
		if ((event_handle)*(size_t *)(char *)&(*i) == (event_handle)*(size_t *)(char *)&ptr) { \
			container.erase(i);	 \
			return true; \
		} \
	} \
	return false; \
}

bool cluster::detach_log(const event_handle _log) {
	detach(dispatch.log, _log);
}

event_handle cluster::on_log (std::function<void(const log_t& _event)> _log) {
	dispatch.log.emplace_back(_log);
	return (event_handle)*(size_t *)(char *)&_log;
}

bool cluster::detach_voice_state_update(const event_handle _voice_state_update) {
	detach(dispatch.voice_state_update, _voice_state_update);
}

event_handle cluster::on_voice_state_update (std::function<void(const voice_state_update_t& _event)> _voice_state_update) {
	dispatch.voice_state_update.emplace_back(_voice_state_update);
	return (event_handle)*(size_t *)(char *)&_voice_state_update;
}

bool cluster::detach_voice_client_disconnect(const event_handle _voice_client_disconnect) {
	detach(dispatch.voice_client_disconnect, _voice_client_disconnect);
}

event_handle cluster::on_voice_client_disconnect (std::function<void(const voice_client_disconnect_t& _event)> _voice_client_disconnect) {
	dispatch.voice_client_disconnect.emplace_back(_voice_client_disconnect);
	return (event_handle)*(size_t *)(char *)&_voice_client_disconnect;
}

bool cluster::detach_voice_client_speaking(const event_handle _voice_client_speaking) {
	detach(dispatch.voice_client_speaking, _voice_client_speaking);
}

event_handle cluster::on_voice_client_speaking (std::function<void(const voice_client_speaking_t& _event)> _voice_client_speaking) {
	dispatch.voice_client_speaking.emplace_back(_voice_client_speaking);;
	return (event_handle)*(size_t *)(char *)&_voice_client_speaking;
}

bool cluster::detach_stage_instance_create(const event_handle _stage_instance_create) {
	detach(dispatch.stage_instance_create, _stage_instance_create);
}

event_handle cluster::on_stage_instance_create (std::function<void(const stage_instance_create_t& _event)> _stage_instance_create) {
	dispatch.stage_instance_create.emplace_back(_stage_instance_create);;
	return (event_handle)*(size_t *)(char *)&_stage_instance_create;
}

bool cluster::detach_stage_instance_update(const event_handle _stage_instance_update) {
	detach(dispatch.stage_instance_update, _stage_instance_update);
}

event_handle cluster::on_stage_instance_update (std::function<void(const stage_instance_update_t& _event)> _stage_instance_update) {
	dispatch.stage_instance_update.emplace_back(_stage_instance_update);;
	return (event_handle)*(size_t *)(char *)&_stage_instance_update;
}

bool cluster::detach_stage_instance_delete(const event_handle _stage_instance_delete) {
	detach(dispatch.stage_instance_delete, _stage_instance_delete);
}

event_handle cluster::on_stage_instance_delete (std::function<void(const stage_instance_delete_t& _event)> _stage_instance_delete) {
	dispatch.stage_instance_delete.emplace_back(_stage_instance_delete);;
	return (event_handle)*(size_t *)(char *)&_stage_instance_delete;
}

bool cluster::detach_interaction_create(const event_handle _interaction_create) {
	detach(dispatch.interaction_create, _interaction_create);
}

event_handle cluster::on_guild_scheduled_event_create (std::function<void(const guild_scheduled_event_create_t& _event)> _guild_scheduled_event_create) {
	dispatch.guild_scheduled_event_create.emplace_back(_guild_scheduled_event_create);
	return (event_handle)*(size_t *)(char *)&_guild_scheduled_event_create;
}

bool cluster::detach_guild_scheduled_event_create(const event_handle _guild_scheduled_event_create) {
	detach(dispatch.guild_scheduled_event_create, _guild_scheduled_event_create);
}

event_handle cluster::on_guild_scheduled_event_update (std::function<void(const guild_scheduled_event_update_t& _event)> _guild_scheduled_event_update) {
	dispatch.guild_scheduled_event_update.emplace_back(_guild_scheduled_event_update);
	return (event_handle)*(size_t *)(char *)&_guild_scheduled_event_update;
}

bool cluster::detach_guild_scheduled_event_update(const event_handle _guild_scheduled_event_update) {
	detach(dispatch.guild_scheduled_event_update, _guild_scheduled_event_update);
}

event_handle cluster::on_guild_scheduled_event_delete (std::function<void(const guild_scheduled_event_delete_t& _event)> _guild_scheduled_event_delete) {
	dispatch.guild_scheduled_event_delete.emplace_back(_guild_scheduled_event_delete);
	return (event_handle)*(size_t *)(char *)&_guild_scheduled_event_delete;
}

bool cluster::detach_guild_scheduled_event_delete(const event_handle _guild_scheduled_event_delete) {
	detach(dispatch.guild_scheduled_event_delete, _guild_scheduled_event_delete);
}

event_handle cluster::on_guild_scheduled_event_user_add (std::function<void(const guild_scheduled_event_user_add_t& _event)> _guild_scheduled_event_user_add) {
	dispatch.guild_scheduled_event_user_add.emplace_back(_guild_scheduled_event_user_add);
	return (event_handle)*(size_t *)(char *)&_guild_scheduled_event_user_add;
}

bool cluster::detach_guild_scheduled_event_user_add(const event_handle _guild_scheduled_event_user_add) {
	detach(dispatch.guild_scheduled_event_user_add, _guild_scheduled_event_user_add);
}

event_handle cluster::on_guild_scheduled_event_user_remove (std::function<void(const guild_scheduled_event_user_remove_t& _event)> _guild_scheduled_event_user_remove) {
	dispatch.guild_scheduled_event_user_remove.emplace_back(_guild_scheduled_event_user_remove);
	return (event_handle)*(size_t *)(char *)&_guild_scheduled_event_user_remove;
}

bool cluster::detach_guild_scheduled_event_user_remove(const event_handle _guild_scheduled_event_user_remove) {
	detach(dispatch.guild_scheduled_event_user_remove, _guild_scheduled_event_user_remove);
}

event_handle cluster::on_interaction_create (std::function<void(const interaction_create_t& _event)> _interaction_create) {
	dispatch.interaction_create.emplace_back(_interaction_create);
	return (event_handle)*(size_t *)(char *)&_interaction_create;
}

bool cluster::detach_button_click(const event_handle _button_click) {
	detach(dispatch.button_click, _button_click);
}

event_handle cluster::on_button_click (std::function<void(const button_click_t& _event)> _button_click) {
	dispatch.button_click.emplace_back(_button_click);;
	return (event_handle)*(size_t *)(char *)&_button_click;
}

bool cluster::detach_autocomplete(const event_handle _autocomplete) {
	detach(dispatch.autocomplete, _autocomplete);
}

event_handle cluster::on_autocomplete (std::function<void(const autocomplete_t& _event)> _autocomplete) {
	dispatch.autocomplete.emplace_back(_autocomplete);;
	return (event_handle)*(size_t *)(char *)&_autocomplete;
}

bool cluster::detach_select_click(const event_handle _select_click) {
	detach(dispatch.select_click, _select_click);
}

event_handle cluster::on_select_click (std::function<void(const select_click_t& _event)> _select_click) {
	dispatch.select_click.emplace_back(_select_click);;
	return (event_handle)*(size_t *)(char *)&_select_click;
}

bool cluster::detach_guild_delete(const event_handle _guild_delete) {
	detach(dispatch.guild_delete, _guild_delete);
}

event_handle cluster::on_guild_delete (std::function<void(const guild_delete_t& _event)> _guild_delete) {
	dispatch.guild_delete.emplace_back(_guild_delete);;
	return (event_handle)*(size_t *)(char *)&_guild_delete;
}

bool cluster::detach_channel_delete(const event_handle _channel_delete) {
	detach(dispatch.channel_delete, _channel_delete);
}

event_handle cluster::on_channel_delete (std::function<void(const channel_delete_t& _event)> _channel_delete) {
	dispatch.channel_delete.emplace_back(_channel_delete);;
	return (event_handle)*(size_t *)(char *)&_channel_delete;
}

bool cluster::detach_channel_update(const event_handle _channel_update) {
	detach(dispatch.channel_update, _channel_update);
}

event_handle cluster::on_channel_update (std::function<void(const channel_update_t& _event)> _channel_update) {
	dispatch.channel_update.emplace_back(_channel_update);;
	return (event_handle)*(size_t *)(char *)&_channel_update;
}

bool cluster::detach_ready(const event_handle _ready) {
	detach(dispatch.ready, _ready);
}

event_handle cluster::on_ready (std::function<void(const ready_t& _event)> _ready) {
	dispatch.ready.emplace_back(_ready);;
	return (event_handle)*(size_t *)(char *)&_ready;
}

bool cluster::detach_message_delete(const event_handle _message_delete) {
	detach(dispatch.message_delete, _message_delete);
}

event_handle cluster::on_message_delete (std::function<void(const message_delete_t& _event)> _message_delete) {
	dispatch.message_delete.emplace_back(_message_delete);;
	return (event_handle)*(size_t *)(char *)&_message_delete;
}

bool cluster::detach_application_command_delete(const event_handle _application_command_delete) {
	detach(dispatch.application_command_delete, _application_command_delete);
}

event_handle cluster::on_application_command_delete (std::function<void(const application_command_delete_t& _event)> _application_command_delete) {
	dispatch.application_command_delete.emplace_back(_application_command_delete);;
	return (event_handle)*(size_t *)(char *)&_application_command_delete;
}

bool cluster::detach_guild_member_remove(const event_handle _guild_member_remove) {
	detach(dispatch.guild_member_remove, _guild_member_remove);
}

event_handle cluster::on_guild_member_remove (std::function<void(const guild_member_remove_t& _event)> _guild_member_remove) {
	dispatch.guild_member_remove.emplace_back(_guild_member_remove);;
	return (event_handle)*(size_t *)(char *)&_guild_member_remove;
}

bool cluster::detach_application_command_create(const event_handle _application_command_create) {
	detach(dispatch.application_command_create, _application_command_create);
}

event_handle cluster::on_application_command_create (std::function<void(const application_command_create_t& _event)> _application_command_create) {
	dispatch.application_command_create.emplace_back(_application_command_create);;
	return (event_handle)*(size_t *)(char *)&_application_command_create;
}

bool cluster::detach_resumed(const event_handle _resumed) {
	detach(dispatch.resumed, _resumed);
}

event_handle cluster::on_resumed (std::function<void(const resumed_t& _event)> _resumed) {
	dispatch.resumed.emplace_back(_resumed);;
	return (event_handle)*(size_t *)(char *)&_resumed;
}

bool cluster::detach_guild_role_create(const event_handle _guild_role_create) {
	detach(dispatch.guild_role_create, _guild_role_create);
}

event_handle cluster::on_guild_role_create (std::function<void(const guild_role_create_t& _event)> _guild_role_create) {
	dispatch.guild_role_create.emplace_back(_guild_role_create);;
	return (event_handle)*(size_t *)(char *)&_guild_role_create;
}

bool cluster::detach_typing_start(const event_handle _typing_start) {
	detach(dispatch.typing_start, _typing_start);
}

event_handle cluster::on_typing_start (std::function<void(const typing_start_t& _event)> _typing_start) {
	dispatch.typing_start.emplace_back(_typing_start);;
	return (event_handle)*(size_t *)(char *)&_typing_start;
}

bool cluster::detach_message_reaction_add(const event_handle _message_reaction_add) {
	detach(dispatch.message_reaction_add, _message_reaction_add);
}

event_handle cluster::on_message_reaction_add (std::function<void(const message_reaction_add_t& _event)> _message_reaction_add) {
	dispatch.message_reaction_add.emplace_back(_message_reaction_add);;
	return (event_handle)*(size_t *)(char *)&_message_reaction_add;
}

bool cluster::detach_guild_members_chunk(const event_handle _guild_members_chunk) {
	detach(dispatch.guild_members_chunk, _guild_members_chunk);
}

event_handle cluster::on_guild_members_chunk (std::function<void(const guild_members_chunk_t& _event)> _guild_members_chunk) {
	dispatch.guild_members_chunk.emplace_back(_guild_members_chunk);;
	return (event_handle)*(size_t *)(char *)&_guild_members_chunk;
}

bool cluster::detach_message_reaction_remove(const event_handle _message_reaction_remove) {
	detach(dispatch.message_reaction_remove, _message_reaction_remove);
}

event_handle cluster::on_message_reaction_remove (std::function<void(const message_reaction_remove_t& _event)> _message_reaction_remove) {
	dispatch.message_reaction_remove.emplace_back(_message_reaction_remove);;
	return (event_handle)*(size_t *)(char *)&_message_reaction_remove;
}

bool cluster::detach_guild_create(const event_handle _guild_create) {
	detach(dispatch.guild_create, _guild_create);
}

event_handle cluster::on_guild_create (std::function<void(const guild_create_t& _event)> _guild_create) {
	dispatch.guild_create.emplace_back(_guild_create);;
	return (event_handle)*(size_t *)(char *)&_guild_create;
}

bool cluster::detach_channel_create(const event_handle _channel_create) {
	detach(dispatch.channel_create, _channel_create);
}

event_handle cluster::on_channel_create (std::function<void(const channel_create_t& _event)> _channel_create) {
	dispatch.channel_create.emplace_back(_channel_create);;
	return (event_handle)*(size_t *)(char *)&_channel_create;
}

bool cluster::detach_message_reaction_remove_emoji(const event_handle _message_reaction_remove_emoji) {
	detach(dispatch.message_reaction_remove_emoji, _message_reaction_remove_emoji);
}

event_handle cluster::on_message_reaction_remove_emoji (std::function<void(const message_reaction_remove_emoji_t& _event)> _message_reaction_remove_emoji) {
	dispatch.message_reaction_remove_emoji.emplace_back(_message_reaction_remove_emoji);;
	return (event_handle)*(size_t *)(char *)&_message_reaction_remove_emoji;
}

bool cluster::detach_message_delete_bulk(const event_handle _message_delete_bulk) {
	detach(dispatch.message_delete_bulk, _message_delete_bulk);
}

event_handle cluster::on_message_delete_bulk (std::function<void(const message_delete_bulk_t& _event)> _message_delete_bulk) {
	dispatch.message_delete_bulk.emplace_back(_message_delete_bulk);;
	return (event_handle)*(size_t *)(char *)&_message_delete_bulk;
}

bool cluster::detach_guild_role_update(const event_handle _guild_role_update) {
	detach(dispatch.guild_role_update, _guild_role_update);
}

event_handle cluster::on_guild_role_update (std::function<void(const guild_role_update_t& _event)> _guild_role_update) {
	dispatch.guild_role_update.emplace_back(_guild_role_update);;
	return (event_handle)*(size_t *)(char *)&_guild_role_update;
}

bool cluster::detach_guild_role_delete(const event_handle _guild_role_delete) {
	detach(dispatch.guild_role_delete, _guild_role_delete);
}

event_handle cluster::on_guild_role_delete (std::function<void(const guild_role_delete_t& _event)> _guild_role_delete) {
	dispatch.guild_role_delete.emplace_back(_guild_role_delete);;
	return (event_handle)*(size_t *)(char *)&_guild_role_delete;
}

bool cluster::detach_channel_pins_update(const event_handle _channel_pins_update) {
	detach(dispatch.channel_pins_update, _channel_pins_update);
}

event_handle cluster::on_channel_pins_update (std::function<void(const channel_pins_update_t& _event)> _channel_pins_update) {
	dispatch.channel_pins_update.emplace_back(_channel_pins_update);;
	return (event_handle)*(size_t *)(char *)&_channel_pins_update;
}

bool cluster::detach_message_reaction_remove_all(const event_handle _message_reaction_remove_all) {
	detach(dispatch.message_reaction_remove_all, _message_reaction_remove_all);
}

event_handle cluster::on_message_reaction_remove_all (std::function<void(const message_reaction_remove_all_t& _event)> _message_reaction_remove_all) {
	dispatch.message_reaction_remove_all.emplace_back(_message_reaction_remove_all);;
	return (event_handle)*(size_t *)(char *)&_message_reaction_remove_all;
}

bool cluster::detach_thread_create(const event_handle _thread_create) {
	detach(dispatch.thread_create, _thread_create);
}

event_handle cluster::on_thread_create (std::function<void(const thread_create_t& _event)> _thread_create) {
	dispatch.thread_create.emplace_back(_thread_create);;
	return (event_handle)*(size_t *)(char *)&_thread_create;
}

bool cluster::detach_thread_update(const event_handle _thread_update) {
	detach(dispatch.thread_update, _thread_update);
}

event_handle cluster::on_thread_update (std::function<void(const thread_update_t& _event)> _thread_update) {
	dispatch.thread_update.emplace_back(_thread_update);;
	return (event_handle)*(size_t *)(char *)&_thread_update;
}

bool cluster::detach_thread_delete(const event_handle _thread_delete) {
	detach(dispatch.thread_delete, _thread_delete);
}

event_handle cluster::on_thread_delete (std::function<void(const thread_delete_t& _event)> _thread_delete) {
	dispatch.thread_delete.emplace_back(_thread_delete);;
	return (event_handle)*(size_t *)(char *)&_thread_delete;
}

bool cluster::detach_thread_list_sync(const event_handle _thread_list_sync) {
	detach(dispatch.thread_list_sync, _thread_list_sync);
}

event_handle cluster::on_thread_list_sync (std::function<void(const thread_list_sync_t& _event)> _thread_list_sync) {
	dispatch.thread_list_sync.emplace_back(_thread_list_sync);;
	return (event_handle)*(size_t *)(char *)&_thread_list_sync;
}

bool cluster::detach_thread_member_update(const event_handle _thread_member_update) {
	detach(dispatch.thread_member_update, _thread_member_update);
}

event_handle cluster::on_thread_member_update (std::function<void(const thread_member_update_t& _event)> _thread_member_update) {
	dispatch.thread_member_update.emplace_back(_thread_member_update);;
	return (event_handle)*(size_t *)(char *)&_thread_member_update;
}

bool cluster::detach_thread_members_update(const event_handle _thread_members_update) {
	detach(dispatch.thread_members_update, _thread_members_update);
}

event_handle cluster::on_thread_members_update (std::function<void(const thread_members_update_t& _event)> _thread_members_update) {
	dispatch.thread_members_update.emplace_back(_thread_members_update);;
	return (event_handle)*(size_t *)(char *)&_thread_members_update;
}

bool cluster::detach_voice_server_update(const event_handle _voice_server_update) {
	detach(dispatch.voice_server_update, _voice_server_update);
}

event_handle cluster::on_voice_server_update (std::function<void(const voice_server_update_t& _event)> _voice_server_update) {
	dispatch.voice_server_update.emplace_back(_voice_server_update);;
	return (event_handle)*(size_t *)(char *)&_voice_server_update;
}

bool cluster::detach_guild_emojis_update(const event_handle _guild_emojis_update) {
	detach(dispatch.guild_emojis_update, _guild_emojis_update);
}

event_handle cluster::on_guild_emojis_update (std::function<void(const guild_emojis_update_t& _event)> _guild_emojis_update) {
	dispatch.guild_emojis_update.emplace_back(_guild_emojis_update);;
	return (event_handle)*(size_t *)(char *)&_guild_emojis_update;
}

bool cluster::detach_guild_stickers_update(const event_handle _guild_stickers_update) {
	detach(dispatch.stickers_update, _guild_stickers_update);
}

event_handle cluster::on_guild_stickers_update (std::function<void(const guild_stickers_update_t& _event)> _guild_stickers_update) {
	dispatch.stickers_update.emplace_back(_guild_stickers_update);;
	return (event_handle)*(size_t *)(char *)&_guild_stickers_update;
}

bool cluster::detach_presence_update(const event_handle _presence_update) {
	detach(dispatch.presence_update, _presence_update);
}

event_handle cluster::on_presence_update (std::function<void(const presence_update_t& _event)> _presence_update) {
	dispatch.presence_update.emplace_back(_presence_update);;
	return (event_handle)*(size_t *)(char *)&_presence_update;
}

bool cluster::detach_webhooks_update(const event_handle _webhooks_update) {
	detach(dispatch.webhooks_update, _webhooks_update);
}

event_handle cluster::on_webhooks_update (std::function<void(const webhooks_update_t& _event)> _webhooks_update) {
	dispatch.webhooks_update.emplace_back(_webhooks_update);;
	return (event_handle)*(size_t *)(char *)&_webhooks_update;
}

bool cluster::detach_guild_member_add(const event_handle _guild_member_add) {
	detach(dispatch.guild_member_add, _guild_member_add);
}

event_handle cluster::on_guild_member_add (std::function<void(const guild_member_add_t& _event)> _guild_member_add) {
	dispatch.guild_member_add.emplace_back(_guild_member_add);;
	return (event_handle)*(size_t *)(char *)&_guild_member_add;
}

bool cluster::detach_invite_delete(const event_handle _invite_delete) {
	detach(dispatch.invite_delete, _invite_delete);
}

event_handle cluster::on_invite_delete (std::function<void(const invite_delete_t& _event)> _invite_delete) {
	dispatch.invite_delete.emplace_back(_invite_delete);;
	return (event_handle)*(size_t *)(char *)&_invite_delete;
}

bool cluster::detach_guild_update(const event_handle _guild_update) {
	detach(dispatch.guild_update, _guild_update);
}

event_handle cluster::on_guild_update (std::function<void(const guild_update_t& _event)> _guild_update) {
	dispatch.guild_update.emplace_back(_guild_update);;
	return (event_handle)*(size_t *)(char *)&_guild_update;
}

bool cluster::detach_guild_integrations_update(const event_handle _guild_integrations_update) {
	detach(dispatch.guild_integrations_update, _guild_integrations_update);
}

event_handle cluster::on_guild_integrations_update (std::function<void(const guild_integrations_update_t& _event)> _guild_integrations_update) {
	dispatch.guild_integrations_update.emplace_back(_guild_integrations_update);;
	return (event_handle)*(size_t *)(char *)&_guild_integrations_update;
}

bool cluster::detach_guild_member_update(const event_handle _guild_member_update) {
	detach(dispatch.guild_member_update, _guild_member_update);
}

event_handle cluster::on_guild_member_update (std::function<void(const guild_member_update_t& _event)> _guild_member_update) {
	dispatch.guild_member_update.emplace_back(_guild_member_update);;
	return (event_handle)*(size_t *)(char *)&_guild_member_update;
}

bool cluster::detach_application_command_update(const event_handle _application_command_update) {
	detach(dispatch.application_command_update, _application_command_update);
}

event_handle cluster::on_application_command_update (std::function<void(const application_command_update_t& _event)> _application_command_update) {
	dispatch.application_command_update.emplace_back(_application_command_update);;
	return (event_handle)*(size_t *)(char *)&_application_command_update;
}

bool cluster::detach_invite_create(const event_handle _invite_create) {
	detach(dispatch.invite_create, _invite_create);
}

event_handle cluster::on_invite_create (std::function<void(const invite_create_t& _event)> _invite_create) {
	dispatch.invite_create.emplace_back(_invite_create);;
	return (event_handle)*(size_t *)(char *)&_invite_create;
}

bool cluster::detach_message_update(const event_handle _message_update) {
	detach(dispatch.message_update, _message_update);
}

event_handle cluster::on_message_update (std::function<void(const message_update_t& _event)> _message_update) {
	dispatch.message_update.emplace_back(_message_update);;
	return (event_handle)*(size_t *)(char *)&_message_update;
}

bool cluster::detach_user_update(const event_handle _user_update) {
	detach(dispatch.user_update, _user_update);
}

event_handle cluster::on_user_update (std::function<void(const user_update_t& _event)> _user_update) {
	dispatch.user_update.emplace_back(_user_update);;
	return (event_handle)*(size_t *)(char *)&_user_update;
}

bool cluster::detach_message_create(const event_handle _message_create) {
	detach(dispatch.message_create, _message_create);
}

event_handle cluster::on_message_create (std::function<void(const message_create_t& _event)> _message_create) {
	dispatch.message_create.emplace_back(_message_create);;
	return (event_handle)*(size_t *)(char *)&_message_create;
}

bool cluster::detach_guild_ban_add(const event_handle _guild_ban_add) {
	detach(dispatch.guild_ban_add, _guild_ban_add);
}

event_handle cluster::on_guild_ban_add (std::function<void(const guild_ban_add_t& _event)> _guild_ban_add) {
	dispatch.guild_ban_add.emplace_back(_guild_ban_add);;
	return (event_handle)*(size_t *)(char *)&_guild_ban_add;
}

bool cluster::detach_guild_ban_remove(const event_handle _guild_ban_remove) {
	detach(dispatch.guild_ban_remove, _guild_ban_remove);
}

event_handle cluster::on_guild_ban_remove (std::function<void(const guild_ban_remove_t& _event)> _guild_ban_remove) {
	dispatch.guild_ban_remove.emplace_back(_guild_ban_remove);;
	return (event_handle)*(size_t *)(char *)&_guild_ban_remove;
}

bool cluster::detach_integration_create(const event_handle _integration_create) {
	detach(dispatch.integration_create, _integration_create);
}

event_handle cluster::on_integration_create (std::function<void(const integration_create_t& _event)> _integration_create) {
	dispatch.integration_create.emplace_back(_integration_create);;
	return (event_handle)*(size_t *)(char *)&_integration_create;
}

bool cluster::detach_integration_update(const event_handle _integration_update) {
	detach(dispatch.integration_update, _integration_update);
}

event_handle cluster::on_integration_update (std::function<void(const integration_update_t& _event)> _integration_update) {
	dispatch.integration_update.emplace_back(_integration_update);;
	return (event_handle)*(size_t *)(char *)&_integration_update;
}

bool cluster::detach_integration_delete(const event_handle _integration_delete) {
	detach(dispatch.integration_delete, _integration_delete);
}

event_handle cluster::on_integration_delete (std::function<void(const integration_delete_t& _event)> _integration_delete) {
	dispatch.integration_delete.emplace_back(_integration_delete);;
	return (event_handle)*(size_t *)(char *)&_integration_delete;
}

bool cluster::detach_voice_buffer_send(const event_handle _voice_buffer_send) {
	detach(dispatch.voice_buffer_send, _voice_buffer_send);
}

event_handle cluster::on_voice_buffer_send (std::function<void(const voice_buffer_send_t& _event)> _voice_buffer_send) {
	dispatch.voice_buffer_send.emplace_back(_voice_buffer_send);;
	return (event_handle)*(size_t *)(char *)&_voice_buffer_send;
}

bool cluster::detach_voice_user_talking(const event_handle _voice_user_talking) {
	detach(dispatch.voice_user_talking, _voice_user_talking);
}

event_handle cluster::on_voice_user_talking (std::function<void(const voice_user_talking_t& _event)> _voice_user_talking) {
	dispatch.voice_user_talking.emplace_back(_voice_user_talking);;
	return (event_handle)*(size_t *)(char *)&_voice_user_talking;
}

bool cluster::detach_voice_ready(const event_handle _voice_ready) {
	detach(dispatch.voice_ready, _voice_ready);
}

event_handle cluster::on_voice_ready (std::function<void(const voice_ready_t& _event)> _voice_ready) {
	dispatch.voice_ready.emplace_back(_voice_ready);;
	return (event_handle)*(size_t *)(char *)&_voice_ready;
}

bool cluster::detach_voice_receive(const event_handle _voice_receive) {
	detach(dispatch.voice_receive, _voice_receive);
}

event_handle cluster::on_voice_receive (std::function<void(const voice_receive_t& _event)> _voice_receive) {
	dispatch.voice_receive.emplace_back(_voice_receive);;
	return (event_handle)*(size_t *)(char *)&_voice_receive;
}

bool cluster::detach_voice_track_marker(const event_handle _voice_track_marker) {
	detach(dispatch.voice_track_marker, _voice_track_marker);
}

event_handle cluster::on_voice_track_marker (std::function<void(const voice_track_marker_t& _event)> _voice_track_marker) {
	dispatch.voice_track_marker.emplace_back(_voice_track_marker);;
	return (event_handle)*(size_t *)(char *)&_voice_track_marker;
}

bool cluster::detach_guild_join_request_delete(const event_handle _guild_join_request_delete) {
	detach(dispatch.guild_join_request_delete, _guild_join_request_delete);
}

event_handle cluster::on_guild_join_request_delete (std::function<void(const guild_join_request_delete_t& _event)> _guild_join_request_delete) {
	dispatch.guild_join_request_delete.emplace_back(_guild_join_request_delete);;
	return (event_handle)*(size_t *)(char *)&_guild_join_request_delete;
}

};
