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
#include <chrono>
#include <iostream>
#include <dpp/json.h>
#include <dpp/discord_webhook_server.h>

namespace dpp {

/**
 * @brief An audit reason for each thread. These are per-thread to make the cluster
 * methods like cluster::get_audit_reason and cluster::set_audit_reason thread safe across
 * multiple threads. You must ensure you set the audit reason on the same thread that makes
 * the request associated with it.
 */
thread_local std::string audit_reason;

/**
 * @brief Make a warning lambda for missing message intents
 *
 * @tparam T type of parameter for the event in the router 
 * @param cl Creating cluster
 * @param required_intent Intent which is required
 * @param message Message to display
 * @return std::function<void(const T&)> Returned lambda
 */
template<typename T> std::function<void(const T&)> make_intent_warning(cluster* cl, const intents required_intent, const std::string& message) {
	return [cl, required_intent, message](const T& event) {
		if (!(cl->intents & required_intent) && event.msg.guild_id) {
			cl->log(ll_warning, message);
		}
	};
}

template <build_type BuildType>
bool validate_configuration() {
#ifdef _DEBUG
	[[maybe_unused]] constexpr build_type expected = build_type::debug;
#else
	[[maybe_unused]] constexpr build_type expected = build_type::release;
#endif
#ifdef _WIN32
	if constexpr (BuildType != build_type::universal && BuildType != expected) {
		MessageBox(
			nullptr,
			"Mismatched Debug/Release configurations between project and dpp.dll.\n"
			"Please ensure both your program and the D++ DLL file are both using the same configuration.\n"
			"The program will now terminate.",
			"D++ Debug/Release mismatch",
			MB_OK | MB_ICONERROR
		);
		/* Use std::runtime_rror here because dpp exceptions use std::string and that would crash when catching, because of ABI */
		throw std::runtime_error("Mismatched Debug/Release configurations between project and dpp.dll");
	}
	return true;
#else
	return true;
#endif
}

template bool DPP_EXPORT validate_configuration<build_type::debug>();

template bool DPP_EXPORT validate_configuration<build_type::release>();

template bool DPP_EXPORT validate_configuration<build_type::universal>();

cluster::cluster(uint32_t pool_threads) : cluster("", 0, NO_SHARDS, 1, 1, false, cache_policy::cpol_none, pool_threads)
{
}

cluster::cluster(const std::string &_token, uint32_t _intents, uint32_t _shards, uint32_t _cluster_id, uint32_t _maxclusters, bool comp, cache_policy_t policy, uint32_t pool_threads)
	: default_gateway("gateway.discord.gg"), rest(nullptr), raw_rest(nullptr), compressed(comp), start_time(0), token(_token), last_identify(time(nullptr) - 5), intents(_intents),
	numshards(_shards), cluster_id(_cluster_id), maxclusters(_maxclusters), rest_ping(0.0), cache_policy(policy), ws_mode(ws_json)
{
	socketengine = create_socket_engine(this);
	pool = std::make_unique<thread_pool>(this, pool_threads > 4 ? pool_threads : 4);
	/* Instantiate REST request queues */
	try {
		/* NOTE: These no longer use threads. This instantiates 16+4 dpp::timer instances. */
		rest = new request_queue(this, 16);
		raw_rest = new request_queue(this, 4);
	}
	catch (std::bad_alloc&) {
		delete rest;
		delete raw_rest;
		throw;
	}

	/* Add checks for missing intents, these emit a one-off warning to the log if bound without the right intents */
	on_message_create.set_warning_callback(
		make_intent_warning<message_create_t>(
			this,
			i_message_content,
			"You have attached an event to cluster::on_message_create() but have not specified the privileged intent dpp::i_message_content. Message content, embeds, attachments, and components on received guild messages will be empty.")
	);
	on_message_update.set_warning_callback(
		make_intent_warning<message_update_t>(
			this,
			i_message_content,
			"You have attached an event to cluster::on_message_update() but have not specified the privileged intent dpp::i_message_content. Message content, embeds, attachments, and components on received guild messages will be empty.")
	);

	/* Add slashcommand callback for named commands. */
#ifndef DPP_NO_CORO
	on_slashcommand([this](const slashcommand_t& event) -> task<void> {
		slashcommand_handler_variant copy;
		{
			std::shared_lock lk(named_commands_mutex);
			auto it = named_commands.find(event.command.get_command_name());
			if (it == named_commands.end()) {
				co_return;
			}
			copy = it->second;
		}
		if (std::holds_alternative<co_slashcommand_handler_t>(copy)) {
			co_await std::get<co_slashcommand_handler_t>(copy)(event);
		} else if (std::holds_alternative<slashcommand_handler_t>(copy)) {
			std::get<slashcommand_handler_t>(copy)(event);
		}
		co_return;
	});
#else
	on_slashcommand([this](const slashcommand_t& event) {
		slashcommand_handler_t copy;
		{
			std::shared_lock lk(named_commands_mutex);
			auto it = named_commands.find(event.command.get_command_name());
			if (it == named_commands.end()) {
				return;
			}
			copy = it->second;
		}
		copy(event);
	});
#endif
}

cluster::~cluster()
{
	delete webhook_server;
	delete rest;
	delete raw_rest;
	this->shutdown();
}

request_queue* cluster::get_rest() {
	return rest;
}

cluster& cluster::enable_webhook_server(const std::string& discord_public_key, const std::string_view address, uint16_t port,  const std::string& ssl_private_key, const std::string& ssl_public_key) {
	webhook_server = new discord_webhook_server(this, discord_public_key, address, port, ssl_private_key, ssl_public_key);
	return *this;
}

request_queue* cluster::get_raw_rest() {
	return raw_rest;
}

cluster& cluster::set_websocket_protocol(websocket_protocol_t mode) {
	if (start_time > 0) {
		throw dpp::logic_exception(err_websocket_proto_already_set, "Cannot change websocket protocol on a started cluster!");
	}
	ws_mode = mode;
	return *this;
}

void cluster::queue_work(int priority, work_unit task) {
	pool->enqueue({priority, task});
}

void cluster::log(dpp::loglevel severity, const std::string &msg) const {
	if (!on_log.empty()) {
		/* Pass to user if they've hooked the event */
		dpp::log_t logmsg(nullptr, 0, msg);
		logmsg.severity = severity;
		logmsg.message = msg;
		size_t pos{0};
		while ((pos = logmsg.message.find(token, pos)) != std::string::npos) {
			logmsg.message.replace(pos, token.length(), "*****");
			pos += 5;
		}
		on_log.call(logmsg);
	}
}

dpp::utility::uptime cluster::uptime()
{
	return dpp::utility::uptime(time(nullptr) - start_time);
}

void cluster::add_reconnect(uint32_t shard_id) {
	reconnections[shard_id] = time(nullptr) + RECONNECT_INTERVAL;
	log(ll_trace, "Reconnecting shard " + std::to_string(shard_id) + " in " + std::to_string(RECONNECT_INTERVAL) + " seconds...");
}

void cluster::start(start_type return_after) {

	if (start_time != 0) {
		throw dpp::logic_exception("Cluster already started");
	}

	auto event_loop = [this]() -> void {
		auto reconnect_monitor = numshards != NO_SHARDS ? start_timer([this](auto t) {
			time_t now = time(nullptr);
			for (auto reconnect = reconnections.begin(); reconnect != reconnections.end(); ++reconnect) {
				auto shard_id = reconnect->first;
				auto shard_reconnect_time = reconnect->second;
				if (now >= shard_reconnect_time) {
					/* This shard needs to be reconnected */
					reconnections.erase(reconnect);
					discord_client* old = shards[shard_id];
					/* These values must be copied to the new connection
					 * to attempt to resume it
					 */
					auto seq_no = old->last_seq;
					auto session_id = old->sessionid;
					log(ll_info, "Reconnecting shard " + std::to_string(shard_id));
					/* Make a new resumed connection based off the old one */
					try {
						if (shards[shard_id] != nullptr) {
							log(ll_trace, "Attempting resume...");
							shards[shard_id] = nullptr;
							shards[shard_id] = new discord_client(*old, seq_no, session_id);
						} else {
							log(ll_trace, "Attempting full reconnection...");
							shards[shard_id] = new discord_client(this, shard_id, numshards, token, intents, compressed, ws_mode);
						}
						/* Delete the old one */
						log(ll_trace, "Attempting to delete old connection...");
						delete old;
						old = nullptr;
						/* Set up the new shard's IO events */
						log(ll_trace, "Running new connection...");
						shards[shard_id]->run();
					}
					catch (const std::exception& e) {
						log(ll_info, "Exception when reconnecting shard " + std::to_string(shard_id) + ": " + std::string(e.what()));
						delete shards[shard_id];
						delete old;
						old = nullptr;
						shards[shard_id] = nullptr;
						add_reconnect(shard_id);
					}
					/* It is not possible to reconnect another shard within the same 5-second window,
					 * due to discords strict rate limiting on shard connections, so we bail out here
					 * and only try another reconnect in the next timer interval. Do not try and make
					 * this support multiple reconnects per loop iteration or Discord will smack us
					 * with the rate limiting clue-by-four.
					 */
					return;
				} else {
					log(ll_trace, "Shard " + std::to_string(shard_id) + " not ready to reconnect yet.");
				}
			}
		}, 5) : 0;
		while (!this->terminating && socketengine.get()) {
			socketengine->process_events();
		}
		if (reconnect_monitor) {
			stop_timer(reconnect_monitor);
		}
	};

	if (on_guild_member_add && !(intents & dpp::i_guild_members)) {
		log(ll_warning, "You have attached an event to cluster::on_guild_member_add() but have not specified the privileged intent dpp::i_guild_members. This event will not fire.");
	}

	if (on_guild_member_remove && !(intents & dpp::i_guild_members)) {
		log(ll_warning, "You have attached an event to cluster::on_guild_member_remove() but have not specified the privileged intent dpp::i_guild_members. This event will not fire.");
	}

	if (on_guild_member_update && !(intents & dpp::i_guild_members)) {
		log(ll_warning, "You have attached an event to cluster::on_guild_member_update() but have not specified the privileged intent dpp::i_guild_members. This event will not fire.");
	}

	if (on_presence_update && !(intents & dpp::i_guild_presences)) {
		log(ll_warning, "You have attached an event to cluster::on_presence_update() but have not specified the privileged intent dpp::i_guild_presences. This event will not fire.");
	}

	if (numshards != NO_SHARDS) {
		/* Start up all shards */
		get_gateway_bot([this, return_after](const auto &response) {

			auto throw_if_not_threaded = [this, return_after](exception_error_code error_id, const std::string &msg) {
				log(ll_critical, msg);
				if (return_after == st_wait) {
					throw dpp::connection_exception(error_id, msg);
				}
			};

			if (response.is_error()) {
				if (response.http_info.status == 401) {
					throw_if_not_threaded(err_unauthorized, "Invalid bot token (401: Unauthorized when getting gateway shard count)");
				} else {
					throw_if_not_threaded(err_auto_shard, "get_gateway_bot: " + response.http_info.body);
				}
				return;
			}
			auto g = std::get<gateway>(response.value);
			log(ll_debug, "Cluster: " + std::to_string(g.session_start_remaining) + " of " + std::to_string(g.session_start_total) + " session starts remaining");
			if (g.session_start_remaining < g.shards || g.shards == 0) {
				throw_if_not_threaded(err_no_sessions_left, "Discord indicates you cannot start enough sessions to boot this cluster! Cluster startup aborted. Try again later.");
				return;
			} else if (g.session_start_max_concurrency == 0) {
				throw_if_not_threaded(err_auto_shard, "Cluster: Could not determine concurrency, startup aborted!");
				return;
			} else if (g.session_start_max_concurrency > 1) {
				log(ll_debug, "Cluster: Large bot sharding; Using session concurrency: " + std::to_string(g.session_start_max_concurrency));
			}
			if (numshards == 0) {
				log(ll_info, "Auto Shard: Bot requires " + std::to_string(g.shards) + std::string(" shard") + ((g.shards > 1) ? "s" : ""));
				numshards = g.shards;
			}
			log(ll_debug, "Starting with " + std::to_string(numshards) + " shards...");
			start_time = time(nullptr);

			for (uint32_t s = 0; s < numshards; ++s) {
				/* Filter out shards that aren't part of the current cluster, if the bot is clustered */
				if (s % maxclusters == cluster_id) {
					/* Each discord_client is inserted into the socket engine when we call run() */
					try {
						this->shards[s] = new discord_client(this, s, numshards, token, intents, compressed, ws_mode);
						this->shards[s]->run();
					}
					catch (const std::exception &e) {
						throw_if_not_threaded(err_cant_start_shard, "Could not start shard " + std::to_string(s) + ": " + std::string(e.what()));
						return;
					}
					/* Stagger the shard startups, pausing every 'session_start_max_concurrency' shards for 5 seconds.
					 * This means that for bots that don't have large bot sharding, any number % 1 is always 0,
					 * so it will pause after every shard. For any with non-zero concurrency it'll pause 5 seconds
					 * after every batch.
					 */
					if (((s + 1) % g.session_start_max_concurrency) == 0) {
						size_t wait_time = 5;
						if (g.session_start_max_concurrency > 1) {
							/* If large bot sharding, be sure to give the batch of shards time to settle */
							bool all_connected = true;
							do {
								all_connected = true;
								for (auto &shard: this->shards) {
									if (!shard.second->ready) {
										all_connected = false;
										std::this_thread::sleep_for(std::chrono::milliseconds(100));
										break;
									}
								}
							} while (all_connected);
						}
						std::this_thread::sleep_for(std::chrono::seconds(wait_time));
					}
				}
			}

			log(ll_debug, "Shards started.");
		});

	} else {
		log(ll_debug, "Starting shardless cluster...");
		/* Without the ready event, we have no user information. This is needed
		 * to register commands etc., so we request it via the API.
		 */
		if (!token.empty()) {
			current_user_get([this](const auto &reply) {
				if (reply.is_error()) {
					throw dpp::connection_exception("Could not fetch user information");
				}
				/* We can implicitly upcast here from user_identified to its parent class, user */
				this->me = std::get<user_identified>(reply.value);
				ready_t r(this, 0, "");
				log(ll_debug, "Shardless cluster started.");
				/* Without shards, on_ready must be manually fired here if it has consumers */
				if (!on_ready.empty()) {
					on_ready.call(r);
				}
			});
		}
	}

	if (!token.empty()) {
		/* Get all active DM channels and map them to user id -> dm id */
		current_user_get_dms([this](const dpp::confirmation_callback_t &completion) {
			if (completion.is_error()) {
				log(dpp::ll_debug, "Failed to get bot DM list");
				return;
			}
			dpp::channel_map dmchannels = std::get<channel_map>(completion.value);
			for (auto &c: dmchannels) {
				for (auto &u: c.second.recipients) {
					set_dm_channel(u, c.second.id);
				}
			}
		});
	}

	if (return_after == st_return) {
		engine_thread = std::thread([this, event_loop]() {
			try {
				dpp::utility::set_thread_name("event_loop");
				event_loop();
			}
			catch (const std::exception& e) {
				log(ll_critical, "Event loop unhandled exception: " + std::string(e.what()));
			}
		});
	} else {
		try {
			event_loop();
		}
		catch (const std::exception& e) {
			log(ll_critical, "Event loop unhandled exception: " + std::string(e.what()));
		}
	}
}

void cluster::shutdown() {
	/* Signal termination */
	terminating = true;

	if (engine_thread.joinable()) {
		/* Join engine_thread if it ever started */
		engine_thread.join();
	}

	{
		std::lock_guard<std::mutex> l(timer_guard);
		while (!this->next_timer.empty()) {
			timer_t cur_timer = std::move(next_timer.top());
			if (cur_timer.on_stop) {
				cur_timer.on_stop(cur_timer.handle);
			}
			next_timer.pop();
		}
		next_timer = {};
	}

	/* Terminate shards */
	for (const auto& sh : shards) {
		delete sh.second;
	}
	shards.clear();
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

/**
 * @brief Given an error exception from nlohmann::json, turn it into a discord-style error object
 * that can be parsed by the get_error() function. Modifies the http request body, moving it to
 * `.get_error().errors[0].reason` in the callback.
 * 
 * @param message Exception message
 * @param rv Request completion data
 * @return json
 */
json error_response(const std::string& message, http_request_completion_t& rv)
{
	json j({
		{"code", rv.status},
		{"errors", {
			{"json", {
				{"0", {
					{"body", {
						{"_errors", {{
							{"code", "JSON_PARSE"},
							{"message", rv.body},
						}}}
					}}
				}}
			}}
		}},
		{"message", message}
	});
	rv.body = j.dump(-1, ' ', false, json::error_handler_t::replace);
	return j;
}

void cluster::post_rest(const std::string &endpoint, const std::string &major_parameters, const std::string &parameters, http_method method, const std::string &postdata, json_encode_t callback, const std::string &filename, const std::string &filecontent, const std::string &filemimetype, const std::string &protocol) {
	rest->post_request(std::make_unique<http_request>(endpoint + (!major_parameters.empty() ? "/" : "") + major_parameters, parameters, [endpoint, callback](http_request_completion_t rv) {
		json j;
		if (rv.error == h_success && !rv.body.empty()) {
			try {
				j = json::parse(rv.body);
			}
			catch (const std::exception &e) {
				j = error_response(e.what(), rv);
			}
		}
		if (callback) {
			callback(j, rv);
		}
	}, postdata, method, get_audit_reason(), filename, filecontent, filemimetype, protocol));
}

void cluster::post_rest_multipart(const std::string &endpoint, const std::string &major_parameters, const std::string &parameters, http_method method, const std::string &postdata, json_encode_t callback, const std::vector<message_file_data> &file_data) {
	std::vector<std::string> file_names{};
	std::vector<std::string> file_contents{};
	std::vector<std::string> file_mimetypes{};

	for(const message_file_data& data : file_data) {
		file_names.push_back(data.name);
		file_contents.push_back(data.content);
		file_mimetypes.push_back(data.mimetype);
	}

	rest->post_request(std::make_unique<http_request>(endpoint + (!major_parameters.empty() ? "/" : "") + major_parameters, parameters, [endpoint, callback](http_request_completion_t rv) {
		json j;
		if (rv.error == h_success && !rv.body.empty()) {
			try {
				j = json::parse(rv.body);
			}
			catch (const std::exception &e) {
				j = error_response(e.what(), rv);
			}
		}
		if (callback) {
			callback(j, rv);
		}
	}, postdata, method, get_audit_reason(), file_names, file_contents, file_mimetypes));
}


void cluster::request(const std::string &url, http_method method, http_completion_event callback, const std::string &postdata, const std::string &mimetype, const std::multimap<std::string, std::string> &headers, const std::string &protocol) {
	raw_rest->post_request(std::make_unique<http_request>(url, callback, method, postdata, mimetype, headers, protocol));
}

gateway::gateway() : shards(0), session_start_total(0), session_start_remaining(0), session_start_reset_after(0), session_start_max_concurrency(0) {
}

gateway& gateway::fill_from_json_impl(nlohmann::json* j) {
	url = string_not_null(j, "url");
	shards = int32_not_null(j, "shards");
	session_start_total = int32_not_null(&((*j)["session_start_limit"]), "total");
	session_start_remaining  = int32_not_null(&((*j)["session_start_limit"]), "remaining");
	session_start_reset_after = int32_not_null(&((*j)["session_start_limit"]), "reset_after");
	session_start_max_concurrency = int32_not_null(&((*j)["session_start_limit"]), "max_concurrency");
	return *this;
}

gateway::gateway(nlohmann::json* j) {
	fill_from_json(j);
}

void cluster::set_presence(const dpp::presence &p) {
	if(p.activities.empty()) {
		log(ll_warning, "An empty presence was passed to set_presence.");
		return;
	}

	json pres = p.to_json();
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

cluster& cluster::set_default_gateway(const std::string &default_gateway_new) {
	default_gateway = default_gateway_new;
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

cluster& cluster::set_request_timeout(uint16_t timeout) {
	request_timeout = timeout;
	return *this;
}

bool cluster::register_command(const std::string &name, const slashcommand_handler_t handler) {
	std::unique_lock lk(named_commands_mutex);
	auto [_, inserted] = named_commands.try_emplace(name, handler);
	return inserted;
}

bool cluster::unregister_command(const std::string &name) {
	std::unique_lock lk(named_commands_mutex);
	return named_commands.erase(name) == 1;
}

size_t cluster::active_requests() {
	return rest->get_active_request_count() + raw_rest->get_active_request_count();
}

};
