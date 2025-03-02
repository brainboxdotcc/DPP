/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * SPDX-License-Identifier: Apache-2.0
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

#include <string_view>
#include <dpp/exception.h>
#include <dpp/isa_detection.h>
#include <dpp/discordvoiceclient.h>
#include <dpp/json.h>
#include "../../dave/encryptor.h"

#include "enabled.h"

namespace dpp {

using namespace std::chrono_literals;

/**
 * @brief How long to wait after deriving new key ratchets to expire the old ones
 */
constexpr dave::decryptor::duration RATCHET_EXPIRY = 10s;

void discord_voice_client::update_ratchets(bool force) {

	if (!mls_state || !mls_state->dave_session) {
		return;
	}

	/**
	 * Update everyone's ratchets including the bot. Whenever a new user joins or a user leaves, this invalidates
	 * all the old ratchets and they are replaced with new ones and the old ones are invalidated after RATCHET_EXPIRY seconds.
	 */
	log(ll_debug, "Updating MLS ratchets for " + std::to_string(dave_mls_user_list.size() + 1) + " user(s)");
	for (const auto& user : dave_mls_user_list) {
		if (user == creator->me.id) {
			continue;
		}
		decryptor_list::iterator decryptor;
		/* New user join/old user leave - insert new ratchets if they don't exist */
		decryptor = mls_state->decryptors.find(user.str());
		if (decryptor == mls_state->decryptors.end()) {
			log(ll_debug, "Inserting decryptor key ratchet for NEW user: " + user.str() + ", protocol version: " + std::to_string(mls_state->dave_session->get_protocol_version()));
			auto [iter, inserted] = mls_state->decryptors.emplace(user.str(), std::make_unique<dpp::dave::decryptor>(*creator));
			decryptor = iter;
		}
		decryptor->second->transition_to_key_ratchet(mls_state->dave_session->get_key_ratchet(user), RATCHET_EXPIRY);
	}

	/* 
	 * Encryptor should always be present on execute transition.
	 * Should we throw error if it's missing here?
	 */
	if (mls_state->encryptor) {
		/* Updating key rachet should always be done on execute transition. Generally after group member add/remove. */
		log(ll_debug, "Setting key ratchet for sending audio...");
		mls_state->encryptor->set_key_ratchet(mls_state->dave_session->get_key_ratchet(creator->me.id));
	}

	/**
	 * https://www.ietf.org/archive/id/draft-ietf-mls-protocol-14.html#name-epoch-authenticators
	 * 9.7. Epoch Authenticators
	 * The main MLS key schedule provides a per-epoch epoch_authenticator. If one member of the group is being impersonated by an active attacker,
	 * the epoch_authenticator computed by their client will differ from those computed by the other group members.
	 */
	std::string old_code = mls_state->privacy_code;
	mls_state->privacy_code = generate_displayable_code(mls_state->dave_session->get_last_epoch_authenticator());
	if (!mls_state->privacy_code.empty() && mls_state->privacy_code != old_code) {
		log(ll_info, "New E2EE Privacy Code: " + mls_state->privacy_code);
	}

}

bool discord_voice_client::handle_frame(const std::string &data, ws_opcode opcode) {
	json j;

	/**
	 * MLS frames come in as type OP_BINARY, we can also reply to them as type OP_BINARY.
	 */
	if (opcode == OP_BINARY && data.size() >= sizeof(dave_binary_header_t)) {

		dave_binary_header_t dave_header(data);

		/* These binaries also contains sequence number we need to save */
		receive_sequence = dave_header.seq;

		switch (dave_header.opcode) {
			case voice_client_dave_mls_external_sender: {
				log(ll_debug, "voice_client_dave_mls_external_sender");

				mls_state->dave_session->set_external_sender(dave_header.get_data());
			}
			break;
			case voice_client_dave_mls_proposals: {
				log(ll_debug, "voice_client_dave_mls_proposals");

				std::optional<std::vector<uint8_t>> response = mls_state->dave_session->process_proposals(dave_header.get_data(), dave_mls_user_list);
				if (response.has_value()) {
					auto r = response.value();
					r.insert(r.begin(), voice_client_dave_mls_commit_message);
					this->write(std::string_view(reinterpret_cast<const char*>(r.data()), r.size()), OP_BINARY);
				}
			}
			break;
			case voice_client_dave_announce_commit_transition: {
				this->mls_state->transition_id = dave_header.get_transition_id();
				log(ll_debug, "voice_client_dave_announce_commit_transition");
				auto r = mls_state->dave_session->process_commit(dave_header.get_data());

				/* 
				 * We need to do recovery here when we failed processing the message
				 */
				if (!std::holds_alternative<dave::roster_map>(r)) {
					log(ll_debug, "Unable to process commit in transition " + std::to_string(this->mls_state->transition_id));

					this->recover_from_invalid_commit_welcome();
					break;
				}

				auto rmap = std::get<dave::roster_map>(r);
				this->process_mls_group_rosters(rmap);

				this->ready_for_transition(data);
			}
			break;
			case voice_client_dave_mls_welcome: {
				this->mls_state->transition_id = dave_header.get_transition_id();
				log(ll_debug, "voice_client_dave_mls_welcome with transition id " + std::to_string(this->mls_state->transition_id));

				/* We should always recognize our own selves, but do we? */
				dave_mls_user_list.insert(this->creator->me.id);

				auto r = mls_state->dave_session->process_welcome(dave_header.get_data(), dave_mls_user_list);

				/* 
				 * We need to do recovery here when we failed processing the message
				 */
				if (!r.has_value()) {
					log(ll_debug, "Unable to process welcome in transition " + std::to_string(this->mls_state->transition_id));

					this->recover_from_invalid_commit_welcome();
					break;
				}

				this->process_mls_group_rosters(r.value());
				this->ready_for_transition(data);
			}
			break;
			default:
				log(ll_debug, "Unexpected DAVE frame opcode");
				log(dpp::ll_trace, "R: " + dpp::utility::debug_dump(reinterpret_cast<const uint8_t*>(data.data()), data.length()));
			break;
		}

		return true;
	}

	try {
		log(dpp::ll_trace, std::string("R: ") + data);
		j = json::parse(data);
	}
	catch (const std::exception &e) {
		log(dpp::ll_error, std::string("discord_voice_client::handle_frame ") + e.what() + ": " + data);
		return true;
	}

	if (j.find("seq") != j.end() && j["seq"].is_number()) {
		/**
		  * Save the sequence number needed for heartbeat and resume payload.
		  *
		  * NOTE: Contrary to the documentation, discord does not seem to send messages with sequence number
		  * in order, should we only save the sequence if it's larger number?
		  */
		receive_sequence = j["seq"].get<int32_t>();
	}

	if (j.find("op") != j.end()) {
		uint32_t op = j["op"];

		switch (op) {
			/* Ping acknowledgement */
			case voice_opcode_connection_heartbeat_ack:
				/* These opcodes do not require a response or further action */
			break;
			case voice_opcode_media_sink:
			case voice_client_flags: {
			}
			break;
			case voice_client_platform: {
				voice_client_platform_t vcp(owner, 0, data);
				vcp.voice_client = this;
				vcp.user_id = snowflake_not_null(&j["d"], "user_id");
				vcp.platform = static_cast<client_platform_t>(int8_not_null(&j["d"], "platform"));
				creator->queue_work(0, [this, vcp]() {
					creator->on_voice_client_platform.call(vcp);
				});
			}
			break;
			case voice_opcode_multiple_clients_connect: {
				/**
				 * @brief The list of users that just joined for DAVE
				 */
				std::set<std::string> joining_dave_users = j["d"]["user_ids"];

				dave_mls_user_list.insert(joining_dave_users.begin(), joining_dave_users.end());

				/* Remove this user from pending remove list if exist */
				for (const auto &user : joining_dave_users) {
					dave_mls_pending_remove_list.erase(dpp::snowflake(user));
				}

				log(ll_debug, "New of clients in voice channel: " + std::to_string(joining_dave_users.size()) + " total is " + std::to_string(dave_mls_user_list.size()));
			}
			break;
			case voice_client_dave_mls_invalid_commit_welcome: {
				this->mls_state->transition_id = j["d"]["transition_id"];
				log(ll_debug, "voice_client_dave_mls_invalid_commit_welcome transition id " + std::to_string(this->mls_state->transition_id));
			}
			break;
			case voice_client_dave_execute_transition: {
				log(ll_debug, "voice_client_dave_execute_transition");
				this->mls_state->transition_id = j["d"]["transition_id"];

				if (this->mls_state->pending_transition.is_pending && this->execute_pending_upgrade_downgrade()) {
					break;
				}

				/*
				 * Execute transition from a commit/welcome message.
				 */
				update_ratchets();
			}
			break;
			/* "The protocol only uses this opcode to indicate when a downgrade to protocol version 0 is upcoming." */
			case voice_client_dave_prepare_transition: {
				this->mls_state->transition_id = j["d"]["transition_id"];
				uint64_t protocol_version = j["d"]["protocol_version"];

				this->mls_state->pending_transition = {this->mls_state->transition_id, protocol_version, true};

				log(ll_debug, "voice_client_dave_prepare_transition version=" + std::to_string(protocol_version) + " for transition " + std::to_string(this->mls_state->transition_id));

				if (this->mls_state->transition_id == 0) {
					this->execute_pending_upgrade_downgrade();
				} else {
					json obj = {
						{ "op", voice_client_dave_transition_ready },
						{
							"d",
							{
								{ "transition_id", this->mls_state->transition_id },
							}
						}
					};
					this->write(obj.dump(-1, ' ', false, json::error_handler_t::replace), OP_TEXT);
				}
			}
			break;
			case voice_client_dave_prepare_epoch: {
				uint64_t protocol_version = j["d"]["protocol_version"];
				uint32_t epoch = j["d"]["epoch"];
				log(ll_debug, "voice_client_dave_prepare_epoch version=" + std::to_string(protocol_version) + " for epoch " + std::to_string(epoch));
				if (epoch == 1) {
					/* An epoch 1 is the start of new dave session, update dave_version */
					dave_version = protocol_version == 1 ? dave_version_1 : dave_version_none;

					this->reinit_dave_mls_group();
				}
			}
			break;
			/* Client Disconnect */
			case voice_opcode_client_disconnect: {
				if (j.find("d") != j.end() && j["d"].find("user_id") != j["d"].end() && !j["d"]["user_id"].is_null()) {
					snowflake u_id = snowflake_not_null(&j["d"], "user_id");

					log(ll_debug, "User left voice channel: " + u_id.str());

					auto it = std::find_if(ssrc_map.begin(), ssrc_map.end(), [&u_id](const auto & p) { return p.second == u_id; });

					if (it != ssrc_map.end()) {
						ssrc_map.erase(it);
					}

					/* Mark this user for remove on immediate upgrade */
					dave_mls_pending_remove_list.insert(u_id);

					if (!creator->on_voice_client_disconnect.empty()) {
						voice_client_disconnect_t vcd(owner, 0, data);
						vcd.voice_client = this;
						vcd.user_id = u_id;
						creator->queue_work(0, [this, vcd]() {
							creator->on_voice_client_disconnect.call(vcd);
						});
					}
				}
			}
			break;
			/* Speaking */
			case voice_opcode_client_speaking: {
				if (j.find("d") != j.end()
				    && j["d"].find("user_id") != j["d"].end() && !j["d"]["user_id"].is_null()
				    && j["d"].find("ssrc") != j["d"].end() && !j["d"]["ssrc"].is_null() && j["d"]["ssrc"].is_number_integer()) {
					uint32_t u_ssrc = j["d"]["ssrc"].get<uint32_t>();
					snowflake u_id = snowflake_not_null(&j["d"], "user_id");
					ssrc_map[u_ssrc] = u_id;

					if (!creator->on_voice_client_speaking.empty()) {
						voice_client_speaking_t vcs(owner, 0, data);
						vcs.voice_client = this;
						vcs.user_id = u_id;
						vcs.ssrc = u_ssrc;
						creator->queue_work(0, [this, vcs]() {
							creator->on_voice_client_speaking.call(vcs);
						});
					}
				}
			}
			break;
			/* Voice resume */
			case voice_opcode_connection_resumed:
				log(ll_debug, "Voice connection resumed");
			break;
			/* Voice HELLO */
			case voice_opcode_connection_hello: {
				if (j.find("d") != j.end() && j["d"].find("heartbeat_interval") != j["d"].end() && !j["d"]["heartbeat_interval"].is_null()) {
					this->heartbeat_interval = j["d"]["heartbeat_interval"].get<uint32_t>();
				}

				/* Reset receive_sequence on HELLO */
				receive_sequence = -1;

				if (!modes.empty()) {
					log(dpp::ll_debug, "Resuming voice session " + this->sessionid + "...");
					json obj = {
						{ "op", voice_opcode_connection_resume },
						{
						  "d",
							{
								{ "server_id", std::to_string(this->server_id) },
								{ "session_id", this->sessionid },
								{ "token", this->token },
								{ "seq_ack", this->receive_sequence },
							}
						}
					};
					this->write(obj.dump(-1, ' ', false, json::error_handler_t::replace), OP_TEXT);
				} else {
					log(dpp::ll_debug, "Connecting new voice session (DAVE: " + std::string(dave_version == dave_version_1 ? "Enabled" : "Disabled") + ")...");
					json obj = {
						{ "op", voice_opcode_connection_identify },
						{
						  "d",
							{
								{ "user_id", creator->me.id },
								{ "server_id", std::to_string(this->server_id) },
								{ "session_id", this->sessionid },
								{ "token", this->token },
								{ "max_dave_protocol_version", dave_version },
							}
						}
					};
					this->write(obj.dump(-1, ' ', false, json::error_handler_t::replace), OP_TEXT);
				}
				this->connect_time = time(nullptr);
			}
			break;
			/* Session description */
			case voice_opcode_connection_description: {
				json &d = j["d"];
				size_t ofs = 0;
				for (auto & c : d["secret_key"]) {
					secret_key[ofs++] = (uint8_t)c;
					if (ofs > secret_key.size() - 1) {
						break;
					}
				}
				has_secret_key = true;

				/* Reset packet_nonce */
				packet_nonce = 1;

				bool ready_now = false;

				if (dave_version != dave_version_none) {
					/* DAVE ready later */

					bool dave_incapable = j["d"]["dave_protocol_version"] != static_cast<uint32_t>(dave_version);
					if (dave_incapable) {
						log(ll_error, "We requested DAVE E2EE but didn't receive it from the server, downgrading...");
						dave_version = dave_version_none;
					}

					/* We told gateway that we got DAVE, stay true to ourselves! */
					this->reinit_dave_mls_group();

					/* Ready now when no upgrade or no DAVE user waiting in the vc */
					ready_now = dave_incapable || dave_mls_user_list.empty();
				} else {
					/* Non-DAVE ready immediately */
					ready_now = true;
				}

				if (ready_now) {
					/* This is needed to start voice receiving and make sure that the start of sending isn't cut off */
					send_silence(20);
					/* Fire on_voice_ready */
					if (!creator->on_voice_ready.empty()) {
						voice_ready_t rdy(owner, 0, data);
						rdy.voice_client = this;
						rdy.voice_channel_id = this->channel_id;
						creator->queue_work(0, [this, rdy]() {
							creator->on_voice_ready.call(rdy);
						});
					}
				}
			}
			break;
			/* Voice ready */
			case voice_opcode_connection_ready: {
				/* Video stream stuff comes in this frame too, but we can't use it (YET!) */
				json &d = j["d"];
				this->ip = d["ip"].get<std::string>();
				this->port = d["port"].get<uint16_t>();
				this->ssrc = d["ssrc"].get<uint64_t>();
				destination = address_t(this->ip, this->port);

				// Modes
				for (auto & m : d["modes"]) {
					this->modes.push_back(m.get<std::string>());
				}
				log(ll_debug, "Voice websocket established; UDP endpoint: " + ip + ":" + std::to_string(port) + " [ssrc=" + std::to_string(ssrc) + "] with " + std::to_string(modes.size()) + " modes");

				dpp::socket newfd = 0;
				if ((newfd = ::socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {

					address_t bind_any;
					if (bind(newfd, bind_any.get_socket_address(), bind_any.size()) < 0) {
						throw dpp::connection_exception(err_bind_failure, "Can't bind() client UDP socket");
					}

					if (!set_nonblocking(newfd, true)) {
						throw dpp::connection_exception(err_nonblocking_failure, "Can't switch voice UDP socket to non-blocking mode!");
					}

					/* Attach new file descriptor to the socket engine */
					this->fd = newfd;

					udp_events = dpp::socket_events(
						fd,
						WANT_READ | WANT_WRITE | WANT_ERROR,
						[this](socket, const struct socket_events &e) { read_ready(); },
						[this](socket, const struct socket_events &e) { write_ready(); },
						[this](socket, const struct socket_events &e, int error_code) {
							this->close();
						}
					);
					owner->socketengine->register_socket(udp_events);

					int bound_port = address_t().get_port(this->fd);
					this->write(json({
						{ "op", voice_opcode_connection_select_protocol },
							{ "d", {
								{ "protocol", "udp" },
								{ "data", {
									{ "address", discover_ip() },
										{ "port", bound_port },
										{ "mode", transport_encryption_protocol }
									}
								}
							}
						}
					}).dump(-1, ' ', false, json::error_handler_t::replace), OP_TEXT);
				}
			}
			break;
			default: {
				log(ll_debug, "Unknown voice opcode " + std::to_string(op) + ": " + data);
			}
			break;
		}
	}
	return true;
}


/*
 * Handle DAVE frame utilities.
 */

void discord_voice_client::ready_for_transition(const std::string &data) {
	if (mls_state == nullptr) {
		/* Impossible! */
		return;
	}

	log(ll_debug, "Ready to execute transition " + std::to_string(mls_state->transition_id));
	json obj = {
		{ "op", voice_client_dave_transition_ready },
		{
			"d",
			{
				{ "transition_id", mls_state->transition_id },
			}
		}
	};
	this->write(obj.dump(-1, ' ', false, json::error_handler_t::replace), OP_TEXT);
	mls_state->pending_transition.id = mls_state->transition_id;

	/* When the included transition ID is 0, the transition is for (re)initialization, and it can be executed immediately. */
	if (mls_state->transition_id == 0) {
		/* Mark state ready and update ratchets the first time */
		update_ratchets();
	}

	if (!mls_state->done_ready) {
		mls_state->done_ready = true;

		if (!creator->on_voice_ready.empty()) {
			voice_ready_t rdy(owner, 0, data);
			rdy.voice_client = this;
			rdy.voice_channel_id = this->channel_id;
			creator->queue_work(0, [this, rdy]() {
				creator->on_voice_ready.call(rdy);
			});
		}
	}
}

void discord_voice_client::recover_from_invalid_commit_welcome() {
	json obj = {
		{"op", voice_client_dave_mls_invalid_commit_welcome},
		{
			"d", {
				"transition_id", mls_state->transition_id
			}
		}
	};
	this->write(obj.dump(-1, ' ', false, json::error_handler_t::replace), OP_TEXT);
	reinit_dave_mls_group();
}

bool discord_voice_client::execute_pending_upgrade_downgrade() {
	if (mls_state == nullptr) {
		/* Who called?? */
		return false;
	}

	bool did_upgrade_downgrade = false;

	if (mls_state->transition_id != mls_state->pending_transition.id) {
		log(ll_debug, "execute_pending_upgrade_downgrade unexpected transition_id, we never received voice_client_dave_prepare_transition event with this id: " + std::to_string(mls_state->transition_id));
	} else if (dave_version != mls_state->pending_transition.protocol_version) {
		dave_version = mls_state->pending_transition.protocol_version == 1 ? dave_version_1 : dave_version_none;

		if (mls_state->pending_transition.protocol_version != 0 && dave_version == dave_version_none) {
			log(ll_debug, "execute_pending_upgrade_downgrade unexpected protocol version: " + std::to_string(mls_state->pending_transition.protocol_version)+ " in transition " + std::to_string(mls_state->transition_id));
		} else {
			log(ll_debug, "execute_pending_upgrade_downgrade upgrade/downgrade successful");
			did_upgrade_downgrade = true;
		}
	}

	mls_state->pending_transition.is_pending = false;
	return did_upgrade_downgrade;
}

void discord_voice_client::reinit_dave_mls_group() {
	/* This method is the beginning of a dave session, do the basics */
	if (dave_version != dave_version_none) {
		if (mls_state == nullptr) {
			mls_state = std::make_unique<dave_state>();
		}

		if (mls_state->dave_session == nullptr) {
			mls_state->dave_session = std::make_unique<dave::mls::session>(
				*creator,
				nullptr, snowflake(), [this](std::string const &s1, std::string const &s2) {
					log(ll_debug, "DAVE: " + s1 + ", " + s2);
				});
		}

		mls_state->dave_session->init(dave::max_protocol_version(), channel_id, creator->me.id, mls_state->mls_key);

		auto key_response = mls_state->dave_session->get_marshalled_key_package();
		key_response.insert(key_response.begin(), voice_client_dave_mls_key_package);
		this->write(std::string_view(reinterpret_cast<const char*>(key_response.data()), key_response.size()), OP_BINARY);

		mls_state->encryptor = std::make_unique<dave::encryptor>(*creator);
	}

	if (mls_state) {
		mls_state->decryptors.clear();
		mls_state->cached_roster_map.clear();
		mls_state->privacy_code.clear();
	}

	/* Remove any user in pending remove from MLS member list */
	for (const auto &user : dave_mls_pending_remove_list) {
		dave_mls_user_list.erase(user);
	}
	dave_mls_pending_remove_list.clear();
}

void discord_voice_client::process_mls_group_rosters(const dave::roster_map &rmap) {
	if (mls_state == nullptr) {
		/* What??? */
		return;
	}

	log(ll_debug, "process_mls_group_rosters of size: " + std::to_string(rmap.size()));

	for (const auto &[k, v] : rmap) {
		bool user_has_key = !v.empty();

		/* Debug log for changed and added keys */
		auto cached_user = mls_state->cached_roster_map.find(k);
		if (cached_user == mls_state->cached_roster_map.end()) {
			log(ll_debug, "Added user to MLS Group: " + std::to_string(k));
		} else if (user_has_key && cached_user->second != v) {
			log(ll_debug, "Changed user key in MLS Group: " + std::to_string(k));
		}

		/*
		 * Remove user from recognized list.
		 * Do not remove user with non-empty key.
		 */
		if (user_has_key) {
			continue;
		}

		dpp::snowflake u_id(k);

		log(ll_debug, "Removed user from MLS Group: " + u_id.str());

		dave_mls_user_list.erase(u_id);
		dave_mls_pending_remove_list.erase(u_id);

		/* Remove this user's key ratchet */
		mls_state->decryptors.erase(u_id);
	}

	mls_state->cached_roster_map = rmap;
}

}

