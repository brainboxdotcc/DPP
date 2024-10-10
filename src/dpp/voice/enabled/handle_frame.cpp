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

bool discord_voice_client::handle_frame(const std::string &data, ws_opcode opcode) {
	json j;

	/**
	 * MLS frames come in as type OP_BINARY, we can also reply to them as type OP_BINARY.
	 */
	if (opcode == OP_BINARY && data.size() >= sizeof(dave_binary_header_t)) {

		dave_binary_header_t dave_header(data);

		switch (dave_header.opcode) {
			case voice_client_dave_mls_external_sender: {
				log(ll_debug, "voice_client_dave_mls_external_sender");

				mls_state->dave_session->set_external_sender(dave_header.get_data());

				mls_state->encryptor = std::make_unique<dave::encryptor>(*creator);
				mls_state->decryptors.clear();
			}
			break;
			case voice_client_dave_mls_proposals: {
				log(ll_debug, "voice_client_dave_mls_proposals");

				std::optional<std::vector<uint8_t>> response = mls_state->dave_session->process_proposals(dave_header.get_data(), dave_mls_user_list);
				if (response.has_value()) {
					auto r = response.value();
					mls_state->cached_commit = r;
					r.insert(r.begin(), voice_client_dave_mls_commit_message);
					this->write(std::string_view(reinterpret_cast<const char*>(r.data()), r.size()), OP_BINARY);
				}
			}
			break;
			case voice_client_dave_announce_commit_transaction: {
				log(ll_debug, "voice_client_dave_announce_commit_transaction");
				auto r = mls_state->dave_session->process_commit(mls_state->cached_commit);
				for (const auto& user : dave_mls_user_list) {
					log(ll_debug, "Setting decryptor key ratchet for user: " + user + ", protocol version: " + std::to_string(mls_state->dave_session->get_protocol_version()));
					dpp::snowflake u{user};
					mls_state->decryptors.emplace(u, std::make_unique<dpp::dave::decryptor>(*creator));
					mls_state->decryptors.find(u)->second->transition_to_key_ratchet(mls_state->dave_session->get_key_ratchet(user));
				}
				mls_state->encryptor->set_key_ratchet(mls_state->dave_session->get_key_ratchet(creator->me.id.str()));

				/**
				 * https://www.ietf.org/archive/id/draft-ietf-mls-protocol-14.html#name-epoch-authenticators
				 * 9.7. Epoch Authenticators
				 * The main MLS key schedule provides a per-epoch epoch_authenticator. If one member of the group is being impersonated by an active attacker,
				 * the epoch_authenticator computed by their client will differ from those computed by the other group members.
				 */
				mls_state->privacy_code = generate_displayable_code(mls_state->dave_session->get_last_epoch_authenticator());
				log(ll_debug, "E2EE Privacy Code: " + mls_state->privacy_code);

				if (!creator->on_voice_ready.empty()) {
					voice_ready_t rdy(nullptr, data);
					rdy.voice_client = this;
					rdy.voice_channel_id = this->channel_id;
					creator->on_voice_ready.call(rdy);
				}
			}
			break;
			case voice_client_dave_mls_welcome: {
				this->mls_state->transition_id = dave_header.get_transition_id();
				log(ll_debug, "voice_client_dave_mls_welcome with transition id " + std::to_string(this->mls_state->transition_id));
				auto r = mls_state->dave_session->process_welcome(dave_header.get_data(), dave_mls_user_list);
				if (r.has_value()) {
					for (const auto& user : dave_mls_user_list) {
						log(ll_debug, "Setting decryptor key ratchet for user: " + user + ", protocol version: " + std::to_string(mls_state->dave_session->get_protocol_version()));
						dpp::snowflake u{user};
						mls_state->decryptors.emplace(u, std::make_unique<dpp::dave::decryptor>(*creator));
						mls_state->decryptors.find(u)->second->transition_to_key_ratchet(mls_state->dave_session->get_key_ratchet(user));
					}
					mls_state->encryptor->set_key_ratchet(mls_state->dave_session->get_key_ratchet(creator->me.id.str()));
				}
				mls_state->privacy_code = generate_displayable_code(mls_state->dave_session->get_last_epoch_authenticator());
				log(ll_debug, "E2EE Privacy Code: " + mls_state->privacy_code);

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
				voice_client_platform_t vcp(nullptr, data);
				vcp.voice_client = this;
				vcp.user_id = snowflake_not_null(&j["d"], "user_id");
				vcp.platform = static_cast<client_platform_t>(int8_not_null(&j["d"], "platform"));
				creator->on_voice_client_platform.call(vcp);
			}
			break;
			case voice_opcode_multiple_clients_connect: {
				dave_mls_user_list = j["d"]["user_ids"];
				log(ll_debug, "Number of clients in voice channel: " + std::to_string(dave_mls_user_list.size()));
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

				if (this->mls_state->pending_transition.is_pending) {
					if (this->mls_state->transition_id != this->mls_state->pending_transition.id) {
						log(ll_debug, "voice_client_dave_execute_transition unexpected transition_id, we never received voice_client_dave_prepare_transition event with this id: " + std::to_string(this->mls_state->pending_transition.id));
					} else {
						dave_version = this->mls_state->pending_transition.protocol_version == 1 ? dave_version_1 : dave_version_none;

						if (this->mls_state->pending_transition.protocol_version != 0 && dave_version == dave_version_none) {
							log(ll_debug, "voice_client_dave_execute_transition unexpected protocol version: " + std::to_string(this->mls_state->pending_transition.protocol_version)+ " in transition " + std::to_string(this->mls_state->pending_transition.id));
						}

						this->mls_state->privacy_code.clear();
						this->dave_mls_user_list.clear();

						this->mls_state->pending_transition.is_pending = false;
					}
				}
			}
			break;
			/* "The protocol only uses this opcode to indicate when a downgrade to protocol version 0 is upcoming." */
			case voice_client_dave_prepare_transition: {
				uint64_t transition_id = j["d"]["transition_id"];
				uint64_t protocol_version = j["d"]["protocol_version"];
				this->mls_state->pending_transition = {transition_id, protocol_version, true};
				log(ll_debug, "voice_client_dave_prepare_transition version=" + std::to_string(protocol_version) + " for transition " + std::to_string(transition_id));

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
			break;
			case voice_client_dave_prepare_epoch: {
				uint64_t protocol_version = j["d"]["protocol_version"];
				uint64_t epoch = j["d"]["epoch"];
				log(ll_debug, "voice_client_dave_prepare_epoch version=" + std::to_string(protocol_version) + " for epoch " + std::to_string(epoch));
				if (epoch == 1) {
					mls_state->dave_session->reset();
					mls_state->dave_session->init(dave::max_protocol_version(), channel_id, creator->me.id.str(), mls_state->mls_key);
					auto key_response = mls_state->dave_session->get_marshalled_key_package();
					key_response.insert(key_response.begin(), voice_client_dave_mls_key_package);
					this->write(std::string_view(reinterpret_cast<const char*>(key_response.data()), key_response.size()), OP_BINARY);
				}
			}
			break;
			/* Client Disconnect */
			case voice_opcode_client_disconnect: {
				if (j.find("d") != j.end() && j["d"].find("user_id") != j["d"].end() && !j["d"]["user_id"].is_null()) {
					snowflake u_id = snowflake_not_null(&j["d"], "user_id");
					auto it = std::find_if(ssrc_map.begin(), ssrc_map.end(),
							       [&u_id](const auto & p) { return p.second == u_id; });

					if (it != ssrc_map.end()) {
						ssrc_map.erase(it);
					}

					auto it_dave = dave_mls_user_list.find(j["d"]["user_id"]);
					if (it_dave != dave_mls_user_list.end()) {
						dave_mls_user_list.erase(it_dave);
					}

					if (!creator->on_voice_client_disconnect.empty()) {
						voice_client_disconnect_t vcd(nullptr, data);
						vcd.voice_client = this;
						vcd.user_id = u_id;
						creator->on_voice_client_disconnect.call(vcd);
					}
				}
			}
			break;
			/* Speaking */
			case voice_opcode_client_speaking:
			/* Client Connect (doesn't seem to work) */
			case voice_opcode_client_connect: {
				if (j.find("d") != j.end()
				    && j["d"].find("user_id") != j["d"].end() && !j["d"]["user_id"].is_null()
				    && j["d"].find("ssrc") != j["d"].end() && !j["d"]["ssrc"].is_null() && j["d"]["ssrc"].is_number_integer()) {
					uint32_t u_ssrc = j["d"]["ssrc"].get<uint32_t>();
					snowflake u_id = snowflake_not_null(&j["d"], "user_id");
					ssrc_map[u_ssrc] = u_id;

					if (!creator->on_voice_client_speaking.empty()) {
						voice_client_speaking_t vcs(nullptr, data);
						vcs.voice_client = this;
						vcs.user_id = u_id;
						vcs.ssrc = u_ssrc;
						creator->on_voice_client_speaking.call(vcs);
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
					if (j["d"]["dave_protocol_version"] != static_cast<uint32_t>(dave_version)) {
						log(ll_error, "We requested DAVE E2EE but didn't receive it from the server, downgrading...");
						dave_version = dave_version_none;
						ready_now = true;
					}

					if (mls_state == nullptr) {
						mls_state = std::make_unique<dave_state>();
					}
					if (mls_state->dave_session == nullptr) {
						mls_state->dave_session = std::make_unique<dave::mls::session>(
							*creator,
							nullptr, "", [this](std::string const& s1, std::string const& s2) {
								log(ll_debug, "Dave session constructor callback: " + s1 + ", " + s2);
							});
						mls_state->dave_session->init(dave::max_protocol_version(), channel_id, creator->me.id.str(), mls_state->mls_key);
					}
					auto key_response = mls_state->dave_session->get_marshalled_key_package();
					key_response.insert(key_response.begin(), voice_client_dave_mls_key_package);
					this->write(std::string_view(reinterpret_cast<const char*>(key_response.data()), key_response.size()), OP_BINARY);
				} else {
					ready_now = true;
				}

				if (ready_now) {
					/* This is needed to start voice receiving and make sure that the start of sending isn't cut off */
					send_silence(20);
					/* Fire on_voice_ready */
					if (!creator->on_voice_ready.empty()) {
						voice_ready_t rdy(nullptr, data);
						rdy.voice_client = this;
						rdy.voice_channel_id = this->channel_id;
						creator->on_voice_ready.call(rdy);
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
				// Modes
				for (auto & m : d["modes"]) {
					this->modes.push_back(m.get<std::string>());
				}
				log(ll_debug, "Voice websocket established; UDP endpoint: " + ip + ":" + std::to_string(port) + " [ssrc=" + std::to_string(ssrc) + "] with " + std::to_string(modes.size()) + " modes");

				dpp::socket newfd = 0;
				if ((newfd = ::socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {

					sockaddr_in servaddr{};
					memset(&servaddr, 0, sizeof(sockaddr_in));
					servaddr.sin_family = AF_INET;
					servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
					servaddr.sin_port = htons(0);

					if (bind(newfd, reinterpret_cast<sockaddr*>(&servaddr), sizeof(servaddr)) < 0) {
						throw dpp::connection_exception(err_bind_failure, "Can't bind() client UDP socket");
					}

					if (!set_nonblocking(newfd, true)) {
						throw dpp::connection_exception(err_nonblocking_failure, "Can't switch voice UDP socket to non-blocking mode!");
					}

					/* Hook poll() in the ssl_client to add a new file descriptor */
					this->fd = newfd;
					this->custom_writeable_fd = [this] { return want_write(); };
					this->custom_readable_fd = [this] { return want_read(); };
					this->custom_writeable_ready = [this] { write_ready(); };
					this->custom_readable_ready = [this] { read_ready(); };

					int bound_port = 0;
					sockaddr_in sin{};
					socklen_t len = sizeof(sin);
					if (getsockname(this->fd, reinterpret_cast<sockaddr *>(&sin), &len) > -1) {
						bound_port = ntohs(sin.sin_port);
					}

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


}
