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
#include <dpp/exception.h>
#include <dpp/cluster.h>
#include <dpp/discordclient.h>
#include <dpp/discordevents.h>
#include <dpp/message.h>
#include <dpp/cache.h>
#include <dpp/once.h>
#include <chrono>
#include <iostream>
#include <dpp/nlohmann/json.hpp>
#include <utility>
#include <dpp/fmt-minimal.h>
#include <algorithm>

namespace dpp {

/**
 * @brief An audit reason for each thread. These are per-thread to make the cluster
 * methods like cluster::get_audit_reason and cluster::set_audit_reason thread safe across
 * multiple threads. You must ensure you set the audit reason on the same thread that makes
 * the request associated with it.
 */
thread_local std::string audit_reason;

event_handle __next_handle = 1;

/**
 * @brief Make a warning lambda for missing message intents
 * 
 * @param cl Creating cluster
 * @param required_intent Intent which is required
 * @param message Message to display
 * @return std::function<void()> Returned lambda
 */
std::function<void()> make_intent_warning(cluster* cl, const intents required_intent, const std::string& message) {
	return [cl, required_intent, message]() {
		if (!(cl->intents & required_intent)) {
			cl->log(ll_warning, message);
		}
	};
}

cluster::cluster(const std::string &_token, uint32_t _intents, uint32_t _shards, uint32_t _cluster_id, uint32_t _maxclusters, bool comp, cache_policy_t policy)
	: rest(nullptr), raw_rest(nullptr), compressed(comp), start_time(0), token(_token), last_identify(time(NULL) - 5), intents(_intents),
	numshards(_shards), cluster_id(_cluster_id), maxclusters(_maxclusters), rest_ping(0.0), cache_policy(policy), ws_mode(ws_json)
	
{
	/* Instantiate REST request queues */
	rest = new request_queue(this);
	raw_rest = new request_queue(this);

	/* Add checks for missing intents, these emit a one-off warning to the log if bound without the right intents */
	std::function<void()> message_intents_warning = make_intent_warning(this, i_message_content, "You have attached an event to cluster::on_message_*() but have not specified the privileged intent dpp::i_message_content. Message content, embeds, attachments, and components on received guild messages will be empty.");
	std::function<void()> member_intents_warning = make_intent_warning(this, i_guild_members, "You have attached an event to cluster::on_guild_member_*() but have not specified the privileged intent dpp::i_guild_members. These events will not fire, and the cache will only fill when a user interacts with the bot or channels.");
	on_message_create.set_warning_callback(message_intents_warning);
	on_message_update.set_warning_callback(message_intents_warning);
	on_guild_member_add.set_warning_callback(member_intents_warning);
	on_guild_member_remove.set_warning_callback(member_intents_warning);
	on_guild_member_update.set_warning_callback(member_intents_warning);
}

cluster::~cluster()
{
	delete rest;
	delete raw_rest;
	/* Free memory for active timers */
	for (auto & t : timer_list) {
		delete t.second;
	}
	for (const auto& sh : shards) {
		log(ll_info, fmt::format("Terminating shard id {}", sh.second->shard_id));
		delete sh.second;
	}
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
	if (!on_log.empty()) {
		/* Pass to user if they've hooked the event */
		dpp::log_t logmsg(nullptr, msg);
		logmsg.severity = severity;
		logmsg.message = msg;
		on_log.call(logmsg);
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
			/* Filter out shards that aren't part of the current cluster, if the bot is clustered */
			if (s % maxclusters == cluster_id) {
				/* Each discord_client spawns its own thread in its run() */
				try {
					this->shards[s] = new discord_client(this, s, numshards, token, intents, compressed, ws_mode);
					this->shards[s]->run();
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

void cluster::post_rest_multipart(const std::string &endpoint, const std::string &major_parameters, const std::string &parameters, http_method method, const std::string &postdata, json_encode_t callback, const std::vector<std::string> &filename, const std::vector<std::string> &filecontent) {
	/* NOTE: This is not a memory leak! The request_queue will free the http_request once it reaches the end of its lifecycle */
	rest->post_request(new http_request(endpoint + "/" + major_parameters, parameters, [endpoint, callback, this](const http_request_completion_t& rv) {
		json j;
		if (rv.error == h_success && !rv.body.empty()) {
			try {
				j = json::parse(rv.body);
			}
			catch (const std::exception &e) {
				/* TODO: Do something clever to handle malformed JSON */
				log(ll_error, fmt::format("post_rest_multipart() to {}: {}", endpoint, e.what()));
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
	url = string_not_null(j, "url");
	shards = int32_not_null(j, "shards");
	session_start_total = int32_not_null(&((*j)["session_start_limit"]), "total");
	session_start_remaining  = int32_not_null(&((*j)["session_start_limit"]), "remaining");
	session_start_reset_after = int32_not_null(&((*j)["session_start_limit"]), "reset_after");
	session_start_max_concurrency = int32_not_null(&((*j)["session_start_limit"]), "max_concurrency");
}

void cluster::set_presence(const dpp::presence &p) {
	json pres = json::parse(p.build_json());
	for (auto& s : shards) {
		if (s.second->is_connected()) {
			s.second->queue_message(s.second->jsonobj_to_string(pres));
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

};
