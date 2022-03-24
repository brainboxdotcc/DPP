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
#include <string>
#include <iostream>
#include <fstream>
#ifndef WIN32
#include <unistd.h>
#include <arpa/inet.h>
#endif
#include <dpp/exception.h>
#include <dpp/discordvoiceclient.h>
#include <dpp/cache.h>
#include <dpp/cluster.h>
#include <thread>
#include <dpp/nlohmann/json.hpp>
#include <dpp/fmt-minimal.h>

#ifdef HAVE_VOICE
#include <sodium.h>
#include <opus/opus.h>
#else
struct OpusDecoder {};
struct OpusEncoder {};
struct OpusRepacketizer {};
#endif

namespace dpp {

std::string external_ip;

/**
 * @brief Represents an RTP packet. Size should always be exactly 12.
 */
struct rtp_header {
	uint16_t constant;
	uint16_t sequence;
	uint32_t timestamp;
	uint32_t ssrc;

	rtp_header(uint16_t _seq, uint32_t _ts, uint32_t _ssrc) : constant(htons(0x8078)), sequence(htons(_seq)), timestamp(htonl(_ts)), ssrc(htonl(_ssrc)) {
	}
};

bool discord_voice_client::sodium_initialised = false;

discord_voice_client::discord_voice_client(dpp::cluster* _cluster, snowflake _channel_id, snowflake _server_id, const std::string &_token, const std::string &_session_id, const std::string &_host)
	   : websocket_client(_host.substr(0, _host.find(":")), _host.substr(_host.find(":") + 1, _host.length()), "/?v=4", OP_TEXT),
	runner(nullptr),
	connect_time(0),
	port(0),
	ssrc(0),
	timescale(1000000),
	paused(false),
#if HAVE_VOICE
	encoder(nullptr),
	decoder(nullptr),
	repacketizer(nullptr),
#endif
	fd(INVALID_SOCKET),
	secret_key(nullptr),
	sequence(0),
	timestamp(0),
	last_timestamp(std::chrono::high_resolution_clock::now()),
	sending(false),
	tracks(0),
	creator(_cluster),
	terminating(false),
#ifdef HAVE_VOICE
	decode_voice_recv(true),
#else
	decode_voice_recv(false),
#endif
	heartbeat_interval(0),
	last_heartbeat(time(nullptr)),
	token(_token),
	sessionid(_session_id),
	server_id(_server_id),
	channel_id(_channel_id)
{
#if HAVE_VOICE
	if (!discord_voice_client::sodium_initialised) {
		if (sodium_init() < 0) {
			throw dpp::voice_exception("discord_voice_client::discord_voice_client; sodium_init() failed");
		}
		discord_voice_client::sodium_initialised = true;
	}
	int opusError = 0;
	encoder = opus_encoder_create(48000, 2, OPUS_APPLICATION_VOIP, &opusError);
	if (opusError) {
		throw dpp::voice_exception("discord_voice_client::discord_voice_client; opus_encoder_create() failed");
	}
	opusError = 0;
	decoder = opus_decoder_create(48000, 2, &opusError);
	if (opusError) {
		throw dpp::voice_exception("discord_voice_client::discord_voice_client; opus_decoder_create() failed");
	}
	repacketizer = opus_repacketizer_create();
	if (!repacketizer) {
		throw dpp::voice_exception("discord_voice_client::discord_voice_client; opus_repacketizer_create() failed");
	}
	this->connect();
#else
	throw dpp::voice_exception("Voice support not enabled in this build of D++");
#endif
}

discord_voice_client::~discord_voice_client()
{
	if (runner) {
		this->terminating = true;
		runner->join();
		delete runner;
		runner = nullptr;
	}
#if HAVE_VOICE
	if (encoder) {
		opus_encoder_destroy(encoder);
		encoder = nullptr;
	}
	if (decoder) {
		opus_decoder_destroy(decoder);
		decoder = nullptr;
	}
	if (repacketizer) {
		opus_repacketizer_destroy(repacketizer);
		repacketizer = nullptr;
	}
#endif
	if (secret_key) {
		delete[] secret_key;
		secret_key = nullptr;
	}
}

bool discord_voice_client::is_ready() {
	return secret_key != nullptr;
}

bool discord_voice_client::is_playing() {
	std::lock_guard<std::mutex> lock(this->stream_mutex);
	return (!this->outbuf.empty());
}

void discord_voice_client::thread_run()
{
	do {
		ssl_client::read_loop();
		ssl_client::close();
		if (!terminating) {
			ssl_client::connect();
			websocket_client::connect();
		}
	} while(!terminating);
}

void discord_voice_client::run()
{
	this->runner = new std::thread(&discord_voice_client::thread_run, this);
	this->thread_id = runner->native_handle();
}

int discord_voice_client::udp_send(const char* data, size_t length)
{
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(this->port);
	servaddr.sin_addr.s_addr = inet_addr(this->ip.c_str());
	return sendto(this->fd, data, (int)length, 0, (const sockaddr*)&servaddr, (int)sizeof(sockaddr_in));
}

int discord_voice_client::udp_recv(char* data, size_t max_length)
{
	return recv(this->fd, data, (int)max_length, 0);
}

bool discord_voice_client::handle_frame(const std::string &data)
{
	log(dpp::ll_trace, std::string("R: ") + data);
	json j;
	
	try {
		j = json::parse(data);
	}
	catch (const std::exception &e) {
		log(dpp::ll_error, std::string("discord_voice_client::handle_frame ") + e.what() + ": " + data);
		return true;
	}

	if (j.find("op") != j.end()) {
		uint32_t op = j["op"];

		switch (op) {
			/* Client Disconnect */
			case 13:
			{
				if (j.find("d") != j.end() && j["d"].find("user_id") != j["d"].end() && !j["d"]["user_id"].is_null())
				{
					snowflake u_id = snowflake_not_null(&j["d"], "user_id");
					auto it = std::find_if(ssrcMap.begin(), ssrcMap.end(),
					   [&u_id](const auto & p) { return p.second == u_id; });

					if (it != ssrcMap.end()) 
						ssrcMap.erase(it);

					if (!creator->on_voice_client_disconnect.empty())
					{
						voice_client_disconnect_t vcd(nullptr, data);
						vcd.voice_client = this;
						vcd.user_id = u_id;
						creator->on_voice_client_disconnect.call(vcd);
					}
				}
			}
			break;
			/* Speaking */ 
			case 5:
			/* Client Connect (doesn't seem to work) */
			case 12:
			{
				if (j.find("d") != j.end() 
					&& j["d"].find("user_id") != j["d"].end() && !j["d"]["user_id"].is_null()
					&& j["d"].find("ssrc") != j["d"].end() && !j["d"]["ssrc"].is_null() && j["d"]["ssrc"].is_number_integer()) 
				{
					uint32_t u_ssrc = j["d"]["ssrc"].get<uint32_t>();
					snowflake u_id = snowflake_not_null(&j["d"], "user_id");
					ssrcMap[u_ssrc] = u_id;

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
			case 9:
				log(ll_debug, "Voice connection resumed");
			break;
			/* Voice HELLO */
			case 8: {
				if (j.find("d") != j.end() && j["d"].find("heartbeat_interval") != j["d"].end() && !j["d"]["heartbeat_interval"].is_null()) {
					this->heartbeat_interval = j["d"]["heartbeat_interval"].get<uint32_t>();
				}

				if (!modes.empty()) {
					log(dpp::ll_debug, "Resuming voice session...");
						json obj = {
						{ "op", 7 },
						{
							"d",
							{
								{ "server_id", std::to_string(this->server_id) },
								{ "session_id", this->sessionid },
								{ "token", this->token },
							}
						}
					};
					this->write(obj.dump());
				} else {
					log(dpp::ll_debug, "Connecting new voice session...");
						json obj = {
						{ "op", 0 },
						{
							"d",
							{
								{ "user_id", creator->me.id },
								{ "server_id", std::to_string(this->server_id) },
								{ "session_id", this->sessionid },
								{ "token", this->token },
							}
						}
					};
					this->write(obj.dump());
				}
				this->connect_time = time(nullptr);
			}
			break;
			/* Session description */
			case 4: {
				json &d = j["d"];
				secret_key = new uint8_t[32];
				size_t ofs = 0;
				for (auto & c : d["secret_key"]) {
					*(secret_key + ofs) = (uint8_t)c;
					ofs++;
					if (ofs > 31) {
						break;
					}
				}

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
			break;
			/* Voice ready */
			case 2: {
				/* Video stream stuff comes in this frame too, but we can't use it (YET!) */
				json &d = j["d"];
				this->ip = d["ip"].get<std::string>();
				this->port = d["port"].get<uint16_t>();
				this->ssrc = d["ssrc"].get<uint64_t>();
				// Modes
				for (auto & m : d["modes"]) {
					this->modes.push_back(m.get<std::string>());
				}
				log(ll_debug, fmt::format("Voice websocket established; UDP endpoint: {}:{} [ssrc={}] with {} modes", ip, port, ssrc, modes.size()));

				external_ip = discover_ip();

				dpp::socket newfd;
				if ((newfd = ::socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {

					sockaddr_in servaddr{};
					memset(&servaddr, 0, sizeof(sockaddr_in));
					servaddr.sin_family = AF_INET;
					servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
					servaddr.sin_port = htons(0);

					if (bind(newfd, (sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
						throw dpp::connection_exception("Can't bind() client UDP socket");
					}
					
#ifdef _WIN32
					u_long mode = 1;
					int result = ioctlsocket(newfd, FIONBIO, &mode);
					if (result != NO_ERROR)
						throw dpp::connection_exception("Can't switch socket to non-blocking mode!");
#else
					int ofcmode;
					ofcmode = fcntl(newfd, F_GETFL, 0);
					ofcmode |= O_NDELAY;
					if (fcntl(newfd, F_SETFL, ofcmode)) {
						throw dpp::connection_exception("Can't switch socket to non-blocking mode!");
					}
#endif
					/* Hook select() in the ssl_client to add a new file descriptor */
					this->fd = newfd;
					this->custom_writeable_fd = std::bind(&discord_voice_client::want_write, this);
					this->custom_readable_fd = std::bind(&discord_voice_client::want_read, this);
					this->custom_writeable_ready = std::bind(&discord_voice_client::write_ready, this);
					this->custom_readable_ready = std::bind(&discord_voice_client::read_ready, this);

					int bound_port = 0;
					sockaddr_in sin;
					socklen_t len = sizeof(sin);
					if (getsockname(this->fd, (sockaddr *)&sin, &len) > -1) {
						bound_port = ntohs(sin.sin_port);
					}

					log(ll_debug, "External IP address: " + external_ip);

					this->write(json({
						{ "op", 1 },
							{ "d", {
								{ "protocol", "udp" },
								{ "data", {
										{ "address", external_ip },
										{ "port", bound_port },
										{ "mode", "xsalsa20_poly1305" }
									}
								}
							}
						}
					}).dump());
				}
			}
			break;
		}
	}
	return true;
}

void discord_voice_client::pause_audio(bool pause) {
	this->paused = pause;
}

bool discord_voice_client::is_paused() {
	return this->paused;
}

float discord_voice_client::get_secs_remaining() {
	std::lock_guard<std::mutex> lock(this->stream_mutex);
	float ret = 0;

	for (const auto& packet : outbuf)
		ret += packet.duration * (timescale / 1000000000.0f);

	return ret;
}

dpp::utility::uptime discord_voice_client::get_remaining() {
	float fp_secs = get_secs_remaining();
	return dpp::utility::uptime((time_t)ceil(fp_secs));
}

void discord_voice_client::stop_audio() {
	std::lock_guard<std::mutex> lock(this->stream_mutex);
	outbuf.clear();
}

void discord_voice_client::send(const char* packet, size_t len, uint64_t duration) {
	std::lock_guard<std::mutex> lock(this->stream_mutex);
	voice_out_packet frame;
	frame.packet = std::string(packet, len);
	frame.duration = duration;
	outbuf.emplace_back(frame);
}

void discord_voice_client::read_ready()
{
#ifdef HAVE_VOICE
	uint8_t buffer[65535];
	int r = this->udp_recv((char*)buffer, sizeof(buffer));

	if (r > 0 && !creator->on_voice_receive.empty()) {
		voice_receive_t vr(nullptr, std::string((const char*)buffer, r));
		vr.voice_client = this;
		vr.audio = nullptr;
		vr.audio_size = 0;

		if (r < 12) 
			return;

		/* Get the User ID of the speaker */
		uint32_t speaker_ssrc;
		std::memcpy(&speaker_ssrc, buffer + 8, sizeof(uint32_t));
		speaker_ssrc = ntohl(speaker_ssrc);
		vr.user_id = ssrcMap[speaker_ssrc];

		/* Nonce is the RTP Header with zero padding */
		uint8_t nonce[24] = { 0 };
		std::memcpy(nonce, buffer, 12);

		/* Skip to the encrypted voice data */
		uint8_t *packet = buffer + 12;

		if (crypto_secretbox_open_easy(packet, packet, r - 12, nonce, secret_key))
			return;

		uint32_t packet_len = r - 12 - crypto_box_MACBYTES;

		if (!(packet[0] == 0xbe && packet[1] == 0xde && packet_len > 4))
			return;
		
		/* Skip the RTP Extensions */
		uint16_t header_extension_len;
		memcpy(&header_extension_len, packet + 2, sizeof(uint16_t));
		header_extension_len = ntohs(header_extension_len);

		size_t offset = 4;
		for (size_t i = 0; i < header_extension_len; i++) {
			uint8_t byte = packet[offset];
			offset++;
			if (byte == 0) 
				continue;
			offset += 1 + (byte >> 4u);
		}
		uint8_t byte = packet[offset];
		if (byte == 0x00 || byte == 0x02) offset++;

		/* We're left with the decrypted opus packet */
		packet = packet + offset;
		packet_len -= (uint32_t)offset;

		if (decode_voice_recv)
		{
			opus_int16 pcm[23040];
			int samples = opus_decode(decoder, packet, packet_len, pcm, 5760, 0);
			if (samples < 0)
				return;
			vr.audio = (uint8_t *)pcm;
			vr.audio_size = samples * 4; // 2 channels, 2 byte samples (2 * 2)
			creator->on_voice_receive.call(vr);
		}
		else
		{
			vr.audio = packet;
			vr.audio_size = packet_len;
			creator->on_voice_receive.call(vr);
		}
	}
#else
	throw dpp::voice_exception("Voice support not enabled in this build of D++");
#endif
}

void discord_voice_client::write_ready()
{
	uint64_t duration = 0;
	bool track_marker_found = false;
	uint64_t bufsize = 0;
	{
		std::lock_guard<std::mutex> lock(this->stream_mutex);
		if (!this->paused && outbuf.size()) {

			if (outbuf[0].packet.size() == 2 && ((uint16_t)(*(outbuf[0].packet.data()))) == AUDIO_TRACK_MARKER) {
				outbuf.erase(outbuf.begin());
				track_marker_found = true;
				if (tracks > 0)
					tracks--;
			}
			if (outbuf.size()) {
				if (this->udp_send(outbuf[0].packet.data(), outbuf[0].packet.length()) == (int)outbuf[0].packet.length()) {
					duration = outbuf[0].duration * timescale;
					outbuf.erase(outbuf.begin());
					bufsize = outbuf.size();
				}
			}
		}
	}
	if (duration) {
		std::chrono::nanoseconds latency = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - last_timestamp);
		std::chrono::nanoseconds sleep_time = std::chrono::nanoseconds(duration) - latency;
		if (sleep_time.count() > 0) {
			std::this_thread::sleep_for(sleep_time);
		}
		last_timestamp = std::chrono::high_resolution_clock::now();
		if (!creator->on_voice_buffer_send.empty()) {
			voice_buffer_send_t snd(nullptr, "");
			snd.buffer_size = (int)bufsize;
			snd.voice_client = this;
			creator->on_voice_buffer_send.call(snd);
		}
	}
	if (track_marker_found) {
		if (!creator->on_voice_track_marker.empty()) {
			voice_track_marker_t vtm(nullptr, "");
			vtm.voice_client = this;
			{
				std::lock_guard<std::mutex> lock(this->stream_mutex);
				if (!track_meta.empty()) {
					vtm.track_meta = track_meta[0];
					track_meta.erase(track_meta.begin());
				}
			}
			creator->on_voice_track_marker.call(vtm);
		}
	}
}

dpp::utility::uptime discord_voice_client::get_uptime()
{
	return dpp::utility::uptime(time(nullptr) - connect_time);
}

bool discord_voice_client::is_connected()
{
	return (this->get_state() == CONNECTED);
}

dpp::socket discord_voice_client::want_write() {
	std::lock_guard<std::mutex> lock(this->stream_mutex);
	if (!this->paused && !outbuf.empty()) {
		return fd;
	} else {
		return INVALID_SOCKET;
	}
}

dpp::socket discord_voice_client::want_read() {
	return fd;
}

void discord_voice_client::error(uint32_t errorcode)
{
	const static std::map<uint32_t, std::string> errortext = {
		{ 1000, "Socket shutdown" },
		{ 1001, "Client is leaving" },
		{ 1002, "Endpoint received a malformed frame" },
		{ 1003, "Endpoint received an unsupported frame" },
		{ 1004, "Reserved code" },
		{ 1005, "Expected close status, received none" },
		{ 1006, "No close code frame has been received" },
		{ 1007, "Endpoint received inconsistent message (e.g. malformed UTF-8)" },
		{ 1008, "Generic error" },
		{ 1009, "Endpoint won't process large frame" },
		{ 1010, "Client wanted an extension which server did not negotiate" },
		{ 1011, "Internal server error while operating" },
		{ 1012, "Server/service is restarting" },
		{ 1013, "Temporary server condition forced blocking client's request" },
		{ 1014, "Server acting as gateway received an invalid response" },
		{ 1015, "Transport Layer Security handshake failure" },
		{ 4001, "Unknown opcode" },
		{ 4002, "Failed to decode payload" },
		{ 4003, "Not authenticated" },
		{ 4004, "Authentication failed" },
		{ 4005, "Already authenticated" },
		{ 4006, "Session no longer valid" },
		{ 4009, "Session timeout" },
		{ 4011, "Server not found" },
		{ 4012, "Unknown protocol" },
		{ 4014, "Disconnected" },
		{ 4015, "Voice server crashed" },
		{ 4016, "Unknown encryption mode" }
	};
	std::string error = "Unknown error";
	auto i = errortext.find(errorcode);
	if (i != errortext.end()) {
		error = i->second;
	}
	log(dpp::ll_warning, fmt::format("Voice session error: {} on channel {}: {}", errorcode, channel_id, error));

	/* Errors 4004...4016 except 4014 are fatal and cause termination of the voice session */
	if (errorcode >= 4003) {
		stop_audio();
		this->terminating = true;
		log(dpp::ll_error, "This is a non-recoverable error, giving up on voice connection");
	}
}

void discord_voice_client::log(dpp::loglevel severity, const std::string &msg) const
{
	creator->log(severity, msg);
}

void discord_voice_client::queue_message(const std::string &j, bool to_front)
{
	std::lock_guard<std::mutex> locker(queue_mutex);
	if (to_front) {
		message_queue.emplace_front(j);
	} else {
		message_queue.emplace_back(j);
	}
}

void discord_voice_client::clear_queue()
{
	std::lock_guard<std::mutex> locker(queue_mutex);
	message_queue.clear();
}

size_t discord_voice_client::get_queue_size()
{
	std::lock_guard<std::mutex> locker(queue_mutex);
	return message_queue.size();
}

const std::vector<std::string> discord_voice_client::get_marker_metadata() {
	std::lock_guard<std::mutex> locker(queue_mutex);
	return track_meta;
}

void discord_voice_client::one_second_timer()
{
	if (terminating) {
		throw dpp::connection_exception("Terminating voice connection");
	}
	/* Rate limit outbound messages, 1 every odd second, 2 every even second */
	if (this->get_state() == CONNECTED) {
		for (int x = 0; x < (time(nullptr) % 2) + 1; ++x) {
			std::lock_guard<std::mutex> locker(queue_mutex);
			if (!message_queue.empty()) {
				std::string message = message_queue.front();
				message_queue.pop_front();
				this->write(message);
			}
		}

		if (this->heartbeat_interval) {
			/* Check if we're due to emit a heartbeat */
			if (time(nullptr) > last_heartbeat + ((heartbeat_interval / 1000.0) * 0.75)) {
				queue_message(json({{"op", 3}, {"d", rand()}}).dump(), true);
				last_heartbeat = time(nullptr);
			}
		}
	}
}

size_t discord_voice_client::encode(uint8_t *input, size_t inDataSize, uint8_t *output, size_t &outDataSize)
{
#if HAVE_VOICE
	outDataSize = 0;
	int mEncFrameBytes = 11520;
	int mEncFrameSize = 2880;
	if (0 == (inDataSize % mEncFrameBytes)) {
		bool isOk = true;
		uint8_t *out = encode_buffer;

		memset(out, 0, sizeof(encode_buffer));
		repacketizer = opus_repacketizer_init(repacketizer);
		if (!repacketizer) {
			log(ll_warning, "opus_repacketizer_init(): failure");
			return outDataSize;
		}
		for (size_t i = 0; i < (inDataSize / mEncFrameBytes); ++ i) {
			const opus_int16* pcm = (opus_int16*)(input + i * mEncFrameBytes);
			int ret = opus_encode(encoder, pcm, mEncFrameSize, out, 65536);
			if (ret > 0) {
				int retval = opus_repacketizer_cat(repacketizer, out, ret);
				if (retval != OPUS_OK) {
					isOk = false;
					log(ll_warning, fmt::format("opus_repacketizer_cat(): {}", opus_strerror(retval)));
					break;
				}
				out += ret;
			} else {
				isOk = false;
				log(ll_warning, fmt::format("opus_encode(): {}", opus_strerror(ret)));
				break;
			}
		}
		if (isOk) {
			int ret = opus_repacketizer_out(repacketizer, output, 65536);
			if (ret > 0) {
				outDataSize = ret;
			} else {
				log(ll_warning, fmt::format("opus_repacketizer_out(): {}", opus_strerror(ret)));
			}
		}
	} else {
		throw dpp::voice_exception(fmt::format("Invalid input data length: {}, must be n times of {}", inDataSize, mEncFrameBytes));
	}
#else
	throw dpp::voice_exception("Voice support not enabled in this build of D++");
#endif
	return outDataSize;
}

void discord_voice_client::insert_marker(const std::string& metadata) {
	/* Insert a track marker. A track marker is a single 16 bit value of 0xFFFF.
	 * This is too small to be a valid RTP packet so the send function knows not
	 * to actually send it, and instead to skip it
	 */
	uint16_t tm = AUDIO_TRACK_MARKER;
	this->send((const char*)&tm, sizeof(uint16_t), 0);
	{
		std::lock_guard<std::mutex> lock(this->stream_mutex);
		track_meta.push_back(metadata);
		tracks++;
	}
}

uint32_t discord_voice_client::get_tracks_remaining() {
	std::lock_guard<std::mutex> lock(this->stream_mutex);
	if (outbuf.empty())
		return 0;
	else
		return tracks + 1;
}

void discord_voice_client::skip_to_next_marker() {
	std::lock_guard<std::mutex> lock(this->stream_mutex);
	/* Keep popping the first entry off the outbuf until the first entry is a track marker */
	while (!outbuf.empty() && outbuf[0].packet.size() != sizeof(uint16_t) && ((uint16_t)(*(outbuf[0].packet.data()))) != AUDIO_TRACK_MARKER) {
		outbuf.erase(outbuf.begin());
	}
	if (outbuf.size()) {
		/* Remove the actual track marker out of the buffer */
		outbuf.erase(outbuf.begin());
	}
	if (tracks > 0)
		tracks--;
	if (!track_meta.empty()) {
		track_meta.erase(track_meta.begin());
	}
}

discord_voice_client& discord_voice_client::send_silence(const uint64_t duration) {
	uint8_t silence_packet[3] = { 0xf8, 0xff, 0xfe };
	send_audio_opus(silence_packet, 3, duration);
	return *this;
}

discord_voice_client& discord_voice_client::send_audio_raw(uint16_t* audio_data, const size_t length)  {
#if HAVE_VOICE
	const size_t max_frame_bytes = 11520;
	if (length > max_frame_bytes) {
		std::string s_audio_data((const char*)audio_data, length);
		while (s_audio_data.length() > max_frame_bytes) {
			std::string packet(s_audio_data.substr(0, max_frame_bytes));
			s_audio_data.erase(s_audio_data.begin(), s_audio_data.begin() + max_frame_bytes);
			if (packet.size() < max_frame_bytes) {
				packet.resize(max_frame_bytes, 0);
			}
			send_audio_raw((uint16_t*)packet.data(), max_frame_bytes);
		}

		return *this;

	}

	opus_int32 encodedAudioMaxLength = (opus_int32)length;
	std::vector<uint8_t> encodedAudioData(encodedAudioMaxLength);
	size_t encodedAudioLength = encodedAudioMaxLength;

	encodedAudioLength = this->encode((uint8_t*)audio_data, length, encodedAudioData.data(), encodedAudioLength);

	send_audio_opus(encodedAudioData.data(), encodedAudioLength);
#else
	throw dpp::voice_exception("Voice support not enabled in this build of D++");
#endif
	return *this;
}

discord_voice_client& discord_voice_client::send_audio_opus(uint8_t* opus_packet, const size_t length) {
#if HAVE_VOICE
	int samples = opus_packet_get_nb_samples(opus_packet, (opus_int32)length, 48000);
	uint64_t duration = (samples / 48) / (timescale / 1000000);
	send_audio_opus(opus_packet, length, duration);
#else
	throw dpp::voice_exception("Voice support not enabled in this build of D++");
#endif
	return *this;
}

discord_voice_client& discord_voice_client::send_audio_opus(uint8_t* opus_packet, const size_t length, uint64_t duration) {
#if HAVE_VOICE
	int frameSize = (int)(48 * duration * (timescale / 1000000));
	opus_int32 encodedAudioMaxLength = (opus_int32)length;
	std::vector<uint8_t> encodedAudioData(encodedAudioMaxLength);
	size_t encodedAudioLength = encodedAudioMaxLength;

	encodedAudioLength = length;
	encodedAudioData.reserve(length);
	memcpy(encodedAudioData.data(), opus_packet, length);

	++sequence;
	const int nonceSize = 24;
	rtp_header header(sequence, timestamp, (uint32_t)ssrc);

	int8_t nonce[nonceSize];
	std::memcpy(nonce, &header, sizeof(header));
	std::memset(nonce + sizeof(header), 0, sizeof(nonce) - sizeof(header));

	std::vector<uint8_t> audioDataPacket(sizeof(header) + encodedAudioLength + crypto_secretbox_MACBYTES);
	std::memcpy(audioDataPacket.data(), &header, sizeof(header));

	crypto_secretbox_easy(audioDataPacket.data() + sizeof(header), encodedAudioData.data(), encodedAudioLength, (const unsigned char*)nonce, secret_key);

	this->send((const char*)audioDataPacket.data(), audioDataPacket.size(), duration);
	timestamp += frameSize;

	speak();
#else
	throw dpp::voice_exception("Voice support not enabled in this build of D++");
#endif
	return *this;
}

discord_voice_client& discord_voice_client::speak() {
	if (!this->sending) {
		this->queue_message(json({
		{"op", 5},
		{"d", {
			{"speaking", 1},
			{"delay", 0},
			{"ssrc", ssrc}
		}}
		}).dump(), true);
		sending = true;
	}
	return *this;
}

discord_voice_client& discord_voice_client::set_timescale(uint64_t new_timescale) {
	timescale = new_timescale;
	return *this;
}

uint64_t discord_voice_client::get_timescale() {
	return timescale;
}

std::string discord_voice_client::discover_ip() {
	dpp::socket newfd = SOCKET_ERROR;
	unsigned char packet[74] = { 0 };
	(*(uint16_t*)(packet)) = htons(0x01);
	(*(uint16_t*)(packet + 2)) = htons(70);
	(*(uint32_t*)(packet + 4)) = htonl((uint32_t)this->ssrc);
	if ((newfd = ::socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
		sockaddr_in servaddr{};
		memset(&servaddr, 0, sizeof(sockaddr_in));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port = htons(0);
		if (bind(newfd, (sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
			log(ll_warning, "Could not bind socket for IP discovery");
			return "";
		}
		memset(&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_port = htons(this->port);
		servaddr.sin_addr.s_addr = inet_addr(this->ip.c_str());
		if (::connect(newfd, (const sockaddr*)&servaddr, sizeof(sockaddr_in)) < 0) {
			log(ll_warning, "Could not connect socket for IP discovery");
			return "";
		}
		if (::send(newfd, (const char*)packet, 74, 0) == -1) {
			log(ll_warning, "Could not send packet for IP discovery");
			return "";
		}
		if (recv(newfd, (char*)packet, 74, 0) == -1) {
			log(ll_warning, "Could not receive packet for IP discovery");
			return "";
		}

		shutdown(newfd, 2);
	#ifdef _WIN32
		if (newfd >= 0 && newfd < FD_SETSIZE) {
			closesocket(newfd);
		}
	#else
		::close(newfd);
	#endif

		//utility::debug_dump(packet, 74);
		return std::string((const char*)(packet + 8));
	}
	return "";
}



};
