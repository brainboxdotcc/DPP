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
#include <utility>
#include <dpp/fmt/format.h>

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
		throw dpp::exception("WSAStartup failure");
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
			throw dpp::exception(fmt::format("Auto Shard: Could not get shard count ({} [code: {}]). Cluster startup aborted.", shardinfo.get_error().message, shardinfo.get_error().code));
		} else {
			throw dpp::exception("Auto Shard: Could not get shard count (unknown error, check your connection). Cluster startup aborted.");
		}
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


void cluster::on_log (std::function<void(const log_t& _event)> _log) {
	this->dispatch.log = _log;
}

void cluster::on_voice_state_update (std::function<void(const voice_state_update_t& _event)> _voice_state_update) {
	this->dispatch.voice_state_update = _voice_state_update;
}

void cluster::on_voice_client_disconnect (std::function<void(const voice_client_disconnect_t& _event)> _voice_client_disconnect) {
	this->dispatch.voice_client_disconnect = _voice_client_disconnect;
}

void cluster::on_voice_client_speaking (std::function<void(const voice_client_speaking_t& _event)> _voice_client_speaking) {
	this->dispatch.voice_client_speaking = _voice_client_speaking;
}

void cluster::on_stage_instance_create (std::function<void(const stage_instance_create_t& _event)> _stage_instance_create) {
	this->dispatch.stage_instance_create = _stage_instance_create;
}

void cluster::on_stage_instance_update (std::function<void(const stage_instance_update_t& _event)> _stage_instance_update) {
	this->dispatch.stage_instance_update = _stage_instance_update;
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

void cluster::on_autocomplete (std::function<void(const autocomplete_t& _event)> _autocomplete) {
	this->dispatch.autocomplete = _autocomplete;
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
