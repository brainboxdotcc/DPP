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
#pragma once

#include <dpp/export.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <fcntl.h>
#include <csignal>
#include <cstring>
#include <string>
#include <map>
#include <vector>
#include <dpp/json_fwd.h>
#include <dpp/wsclient.h>
#include <dpp/dispatcher.h>
#include <dpp/cluster.h>
#include <dpp/discordevents.h>
#include <dpp/socket.h>
#include <queue>
#include <thread>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <future>
#include <functional>
#include <chrono>
#include <set>

struct OpusDecoder;
struct OpusEncoder;
struct OpusRepacketizer;

namespace dpp {

/**
 * @brief Sample rate for OPUS (48khz)
 */
[[maybe_unused]] inline constexpr int32_t opus_sample_rate_hz = 48000;

/**
 * @brief Channel count for OPUS (stereo)
 */
[[maybe_unused]] inline constexpr int32_t opus_channel_count = 2;

/**
 * @brief Discord voice protocol version
 */
[[maybe_unused]] inline constexpr uint8_t voice_protocol_version = 8;


class audio_mixer;

namespace dave::mls {
	class Session;
}

// !TODO: change these to constexpr and rename every occurrence across the codebase
#define AUDIO_TRACK_MARKER (uint16_t)0xFFFF

#define AUDIO_OVERLAP_SLEEP_SAMPLES 30

inline constexpr size_t send_audio_raw_max_length = 11520;

inline constexpr size_t secret_key_size = 32;

struct dave_state;

/*
* @brief For holding a moving average of the number of current voice users, for applying a smooth gain ramp.
*/
struct DPP_EXPORT moving_averager {
	moving_averager() = default;

	moving_averager(uint64_t collection_count_new);

	moving_averager operator+=(int64_t value);

	operator float();

protected:
	std::deque<int64_t> values{};
	uint64_t collectionCount{};
};

// Forward declaration
class cluster;

/**
 * @brief An opus-encoded RTP packet to be sent out to a voice channel
 */
struct DPP_EXPORT voice_out_packet {
	/** 
	 * @brief Each string is a UDP packet.
	 * Generally these will be RTP.
	 */
	std::string packet;

	/**
	 * @brief Duration of packet
	 */
	uint64_t duration;
};

/**
 * @brief Supported DAVE (Discord Audio Visual Encryption) protocol versions
 */
enum dave_version_t : uint8_t {
	/**
	 * @brief DAVE disabled (default for now)
	 */
	dave_version_none = 0,
	/**
	 * @brief DAVE enabled, version 1 (E2EE encryption on top of sodium)
	 */
	dave_version_1 = 1,
};

/**
 * @brief Discord voice websocket opcode types
 */
enum voice_websocket_opcode_t : uint8_t {
	voice_opcode_connection_identify = 0,
	voice_opcode_connection_select_protocol = 1,
	voice_opcode_connection_ready = 2,
	voice_opcode_connection_heartbeat = 3,
	voice_opcode_connection_description = 4,
	voice_opcode_client_speaking = 5,
	voice_opcode_connection_heartbeat_ack = 6,
	voice_opcode_connection_resume = 7,
	voice_opcode_connection_hello = 8,
	voice_opcode_connection_resumed = 9,
	voice_opcode_multiple_clients_connect = 11,
	voice_opcode_client_connect = 12,
	voice_opcode_client_disconnect = 13,
	voice_opcode_media_sink = 15,
	voice_client_flags = 18,
	voice_client_platform = 20,
	voice_client_dave_prepare_transition = 21,
	voice_client_dave_execute_transition = 22,
	voice_client_dave_transition_ready = 23,
	voice_client_dave_prepare_epoch = 24,
	voice_client_dave_mls_external_sender = 25,
	voice_client_dave_mls_key_package = 26,
	voice_client_dave_mls_proposals = 27,
	voice_client_dave_mls_commit_message = 28,
	voice_client_dave_announce_commit_transaction = 29,
	voice_client_dave_mls_welcome = 30,
	voice_client_dave_mls_invalid_commit_welcome = 31,
};

/**
 * @brief DAVE E2EE Binary frame header
 */
#pragma pack(push, 1)
struct dave_binary_header_t {
	/**
	 * @brief Sequence number
	 */
	uint16_t seq;
	/**
	 * @brief Opcode type
	 */
	uint8_t opcode;
	/**
	 * @brief Data package
	 */
	uint8_t package[];

	/**
	 * Get the data package from the packed binary frame, as a vector of uint8_t
	 * for use in the libdave functions
	 *
	 * @param length Length of the data, use the websocket frame size here
	 * @return data blob
	 */
	[[nodiscard]] std::vector<uint8_t> get_data(size_t length) const;

	/**
	 * Get the data package from the packed binary frame for ProcessWelcome,
	 * as a vector of uint8_t for use in the libdave functions.
	 *
	 * @param length Length of the data, use the websocket frame size here
	 * @return data blob
	 */
	[[nodiscard]] std::vector<uint8_t> get_welcome_data(size_t length) const;

	/**
	 * Get transition ID for ProcessWelcome
	 *
	 * @return Transition ID
	 */
	[[nodiscard]] uint16_t get_welcome_transition_id() const;
};
#pragma pack(pop)

using privacy_code_callback_t = std::function<void(const std::string&)>;

/** @brief Implements a discord voice connection.
 * Each discord_voice_client connects to one voice channel and derives from a websocket client.
 */
class DPP_EXPORT discord_voice_client : public websocket_client
{
	/**
	 * @brief Clean up resources
	 */
	void cleanup();

	/**
	 * @brief Mutex for outbound packet stream
	 */
	std::mutex stream_mutex;

	/**
	 * @brief Mutex for message queue
	 */
	std::shared_mutex queue_mutex;

	/**
	 * @brief Queue of outbound messages
	 */
	std::deque<std::string> message_queue;

	/**
	 * @brief Thread this connection is executing on
	 */
	std::thread* runner;

	/**
	 * @brief Run shard loop under a thread
	 */
	void thread_run();

	/**
	 * @brief Last connect time of voice session
	 */
	time_t connect_time;

	/*
	* @brief For mixing outgoing voice data.
	*/
	std::unique_ptr<audio_mixer> mixer;

	/**
	 * @brief IP of UDP/RTP endpoint
	 */
	std::string ip;

	/**
	 * @brief Port number of UDP/RTP endpoint
	 */
	uint16_t port;

	/**
	 * @brief SSRC value 
	 */
	uint64_t ssrc;

	/**
	 * @brief List of supported audio encoding modes
	 */
	std::vector<std::string> modes;

	/**
	 * @brief Timescale in nanoseconds
	 */
	uint64_t timescale;

	/**
	 * @brief Output buffer
	 */
	std::vector<voice_out_packet> outbuf;

	/**
	 * @brief Data type of RTP packet sequence number field.
	 */
	using rtp_seq_t = uint16_t;
	using rtp_timestamp_t = uint32_t;

	/**
	 * @brief Keeps track of the voice payload to deliver to voice handlers.
	 */
	struct voice_payload {
		/**
		 * @brief The sequence number of the RTP packet that generated this
		 * voice payload.
		 */
		rtp_seq_t seq;

		/**
		 * @brief The timestamp of the RTP packet that generated this voice
		 * payload.
		 *
		 * The timestamp is used to detect the order around where sequence
		 * number wraps around.
		 */
		rtp_timestamp_t timestamp;

		/**
		 * @brief The event payload that voice handlers receive.
		 */
		std::unique_ptr<voice_receive_t> vr;
		
		/**
		 * @brief For priority_queue sorting.
		 * @return true if "this" has lower priority that "other",
		 *         i.e. appears later in the queue; false otherwise.
		 */
		bool operator<(const voice_payload& other) const;
	};

	struct voice_payload_parking_lot {
		/**
		 * @brief The range of RTP packet sequence number and timestamp in the lot.
		 *
		 * The minimum is used to drop packets that arrive too late. Packets
		 * less than the minimum have been delivered to voice handlers and
		 * there is no going back. Unfortunately we just have to drop them.
		 *
		 * The maximum is used, at flush time, to calculate the minimum for
		 * the next batch. The maximum is also updated every time we receive an
		 * RTP packet with a larger value.
		 */
		struct seq_range_t {
			rtp_seq_t min_seq, max_seq;
			rtp_timestamp_t min_timestamp, max_timestamp;
		} range;

		/**
		 * @brief The queue of parked voice payloads.
		 * 
		 * We group payloads and deliver them to handlers periodically as the
		 * handling of out-of-order RTP packets. Payloads in between flushes
		 * are parked and sorted in this queue.
		 */
		std::priority_queue<voice_payload> parked_payloads;

		/**
		 * @brief The decoder ctls to be set on the decoder.
		 */
		std::vector<std::function<void(OpusDecoder&)>> pending_decoder_ctls;

		/**
		 * @brief libopus decoder
		 *
		 * Shared with the voice courier thread that does the decoding.
		 * This is not protected by a mutex because only the courier thread
		 * uses the decoder.
		 */
		std::shared_ptr<OpusDecoder> decoder;
	};
	/**
	 * @brief Thread used to deliver incoming voice data to handlers.
	 */
	std::thread voice_courier;

	/**
	 * @brief Shared state between this voice client and the courier thread.
	 */
	struct courier_shared_state_t {
		/**
		 * @brief Protects all following members.
		 */
		std::mutex mtx;

		/**
		 * @brief Signaled when there is a new payload to deliver or terminating state has changed.
		 */
		std::condition_variable signal_iteration;

		/**
		 * @brief Voice buffers to be reported to handler, grouped by speaker.
		 *
		 * Buffers are parked here and flushed every 500ms.
		 */
		std::map<snowflake, voice_payload_parking_lot> parked_voice_payloads;

		/**
		 * @brief Used to signal termination.
		 *
		 * @note Pending payloads are delivered first before termination.
		 */
		bool terminating = false;
	} voice_courier_shared_state;

	/**
	 * @brief The run loop of the voice courier thread.
	 */
	static void voice_courier_loop(discord_voice_client&, courier_shared_state_t&);

	/**
	 * @brief If true, audio packet sending is paused
	 */
	bool paused;

#ifdef HAVE_VOICE
	/**
	 * @brief libopus encoder
	 */
	OpusEncoder* encoder;

	/**
	 * @brief libopus repacketizer
	 * (merges frames into one packet)
	 */
	OpusRepacketizer* repacketizer;

	std::unique_ptr<dave_state> mls_state;

#else
	/**
	 * @brief libopus encoder
	 */
	void* encoder;

	/**
	 * @brief libopus repacketizer
	 * (merges frames into one packet)
	 */
	void* repacketizer;

	std::unique_ptr<int> mls_state{};
#endif

	std::set<std::string> dave_mls_user_list;

	/**
	 * @brief File descriptor for UDP connection
	 */
	dpp::socket fd;

	/**
	 * @brief Secret key for encrypting voice.
	 * If it has been sent, this contains a sequence of exactly 32 bytes
	 * (secret_key_size) and has_secret_key is set to true.
	 */
	std::array<uint8_t, secret_key_size> secret_key;

	/**
	 * @brief True if the voice client has a secret key
	 */
	bool has_secret_key{false};

	/**
	 * @brief Sequence number of outbound audio. This is incremented
	 * once per frame sent.
	 */
	uint16_t sequence;

	/**
	 * @brief Last received sequence from gateway.
	 *
	 * Needed for heartbeat and resume payload.
	 */
	int32_t receive_sequence;

	/**
	 * @brief Timestamp value used in outbound audio. Each packet
	 * has the timestamp value which is incremented to match
	 * how many frames are sent.
	 */
	uint32_t timestamp;

	/**
	 * @brief Each packet should have a nonce, a 32-bit incremental
	 * integer value appended to payload.
	 *
	 * We should keep track of this value and increment it for each
	 * packet sent.
	 *
	 * Current initial value is hardcoded to 1.
	 */
	uint32_t packet_nonce;

	/**
	 * @brief Last sent packet high-resolution timestamp
	 */
	std::chrono::high_resolution_clock::time_point last_timestamp;

	/**
	 * @brief Fraction of the sleep that was not executed after the last audio packet was sent
	 */
	std::chrono::nanoseconds last_sleep_remainder;

	/**
	 * @brief Maps receiving ssrc to user id
	 */
	std::unordered_map<uint32_t, snowflake> ssrc_map;

	/**
	 * @brief This is set to true if we have started sending audio.
	 * When this moves from false to true, this causes the
	 * client to send the 'talking' notification to the websocket.
	 */
	bool sending;

	/**
	 * @brief Number of track markers in the buffer. For example if there
	 * are two track markers in the buffer there are 3 tracks.
	 * 
	 * **Special case:**
	 * 
	 * If the buffer is empty, there are zero tracks in the
	 * buffer.
	 */
	uint32_t tracks;

	/**
	 * @brief Meta data associated with each track.
	 * Arbitrary string that the user can set via
	 * dpp::discord_voice_client::add_marker
	 */
	std::vector<std::string> track_meta;

	/** 
	 * @brief Encoding buffer for opus repacketizer and encode
	 */
	uint8_t encode_buffer[65536];

	/**
	 * @brief DAVE - Discord Audio Visual Encryption
	 * Used for E2EE encryption. dave_protocol_none is
	 * the default right now.
	 * @warning DAVE E2EE is an EXPERIMENTAL feature!
	 */
	dave_version_t dave_version;

	/**
	 * @brief Our public IP address
	 */
	static std::string external_ip;



		/**
	 * @brief Send data to UDP socket immediately.
	 * 
	 * @param data data to send
	 * @param length length of data to send
	 * @return int bytes sent. Will return -1 if we cannot send
	 */
	int udp_send(const char* data, size_t length);

	/**
	 * @brief Receive data from UDP socket immediately.
	 * 
	 * @param data data to receive
	 * @param max_length size of data receiving buffer
	 * @return int bytes received. -1 if there is an error
	 * (e.g. EAGAIN)
	 */
	int udp_recv(char* data, size_t max_length);

	/**
	 * @brief This hooks the ssl_client, returning the file
	 * descriptor if we want to send buffered data, or
	 * -1 if there is nothing to send
	 * 
	 * @return int file descriptor or -1
	 */
	dpp::socket want_write();

	/**
	 * @brief This hooks the ssl_client, returning the file
	 * descriptor if we want to receive buffered data, or
	 * -1 if we are not wanting to receive
	 * 
	 * @return int file descriptor or -1
	 */
	dpp::socket want_read();

	/**
	 * @brief Called by ssl_client when the socket is ready
	 * for writing, at this point we pick the head item off
	 * the buffer and send it. So long as it doesn't error
	 * completely, we pop it off the head of the queue.
	 */
	void write_ready();

	/**
	 * @brief Called by ssl_client when there is data to be
	 * read. At this point we insert that data into the
	 * input queue.
	 * @throw dpp::voice_exception if voice support is not compiled into D++
	 */
	void read_ready();

	/**
	 * @brief Send data to the UDP socket, using the buffer.
	 * 
	 * @param packet packet data
	 * @param len length of packet
	 * @param duration duration of opus packet
	 */
	void send(const char* packet, size_t len, uint64_t duration);

	/**
	 * @brief Queue a message to be sent via the websocket
	 * 
	 * @param j The JSON data of the message to be sent
	 * @param to_front If set to true, will place the message at the front of the queue not the back
	 * (this is for urgent messages such as heartbeat, presence, so they can take precedence over
	 * chunk requests etc)
	 */
	void queue_message(const std::string &j, bool to_front = false);

	/**
	 * @brief Clear the outbound message queue
	 * 
	 */
	void clear_queue();

	/**
	 * @brief Get the size of the outbound message queue
	 * 
	 * @return The size of the queue
	 */
	size_t get_queue_size();

	/**
	 * @brief Encode a byte buffer using opus codec.
	 * Multiple opus frames (2880 bytes each) will be encoded into one packet for sending.
	 * 
	 * @param input Input data as raw bytes of PCM data
	 * @param inDataSize Input data length
	 * @param output Output data as an opus encoded packet
	 * @param outDataSize Output data length, should be at least equal to the input size.
	 * Will be adjusted on return to the actual compressed data size.
	 * @return size_t The compressed data size that was encoded.
	 * @throw dpp::voice_exception If data length to encode is invalid or voice support not compiled into D++
	 */
	size_t encode(uint8_t *input, size_t inDataSize, uint8_t *output, size_t &outDataSize);

public:

	/**
	 * @brief Owning cluster
	 */
	class dpp::cluster* creator;

	/**
	 * @brief This needs to be static, we only initialise libsodium once per program start,
	 * so initialising it on first use in a voice connection is best.
	 */
	static bool sodium_initialised;

	/**
	 * @brief True when the thread is shutting down
	 */
	bool terminating;

	/**
	 * @brief The gain value for the end of the current voice iteration.
	 */
	float end_gain;

	/**
	 * @brief The gain value for the current voice iteration.
	 */
	float current_gain;

	/**
	 * @brief The amount to increment each successive sample for, for the current voice iteration.
	 */
	float increment;

	/**
	 * @brief Heartbeat interval for sending heartbeat keepalive
	 */
	uint32_t heartbeat_interval;

	/**
	 * @brief Last voice channel websocket heartbeat
	 */
	time_t last_heartbeat;

	/**
	 * @brief Thread ID
	 */
	std::thread::native_handle_type thread_id;

	/**
	 * @brief Discord voice session token
	 */
	std::string token;

	/**
	 * @brief Discord voice session id
	 */
	std::string sessionid;

	/**
	 * @brief Server ID
	 */
	snowflake server_id;

	/**
	 * @brief Moving averager.
	 */
	moving_averager moving_average;

	/**
	 * @brief Channel ID
	 */
	snowflake channel_id;

	/**
	 * @brief The audio type to be sent.
	 *
	 * @note On Windows, the default type is overlap audio.
	 * On all other platforms, it is recorded audio.
	 *
	 * If the audio is recorded, the sending of audio packets is throttled.
	 * Otherwise, if the audio is live, the sending is not throttled.
	 *
	 * Discord voice engine is expecting audio data as if they were from
	 * some audio device, e.g. microphone, where the data become available
	 * as they get captured from the audio device.
	 *
	 * In case of recorded audio, unlike from a device, the audio data are
	 * usually instantly available in large chunks. Throttling is needed to
	 * simulate audio data coming from an audio device. In case of live audio,
	 * the throttling is by nature, so no extra throttling is needed.
	 *
	 * Using live audio mode for recorded audio can cause Discord to skip
	 * audio data because Discord does not expect to receive, say, 3 minutes'
	 * worth of audio data in 1 second.
	 *
	 * There are some inaccuracies in the throttling method used by the recorded
	 * audio mode on some systems (mainly Windows) which causes gaps and stutters
	 * in the resulting audio stream. The overlap audio mode provides a different 
	 * implementation that fixes the issue. This method is slightly more CPU 
	 * intensive, and should only be used if you encounter issues with recorded audio 
	 * on your system.
	 * 
	 * Use discord_voice_client::set_send_audio_type to change this value as
	 * it ensures thread safety.
	 */
	enum send_audio_type_t
	{
		satype_recorded_audio,
		satype_live_audio,
		satype_overlap_audio
	} send_audio_type =
#ifdef _WIN32
	satype_overlap_audio;
#else
	satype_recorded_audio;
#endif

	/**
	 * @brief Sets the gain for the specified user.
	 *
	 * Similar to the User Volume slider, controls the listening volume per user.
	 * Uses native Opus gain control, so clients don't have to perform extra
	 * audio processing.
	 *
	 * The gain setting will affect the both individual and combined voice audio.
	 *
	 * The gain value can also be set even before the user connects to the voice
	 * channel.
	 *
	 * @param user_id The ID of the user where the gain is to be controlled.
	 * @param factor Nonnegative factor to scale the amplitude by, where 1.f reverts
	 *               to the default volume.
	 */
	void set_user_gain(snowflake user_id, float factor);

	/**
	 * @brief Log a message to whatever log the user is using.
	 * The logged message is passed up the chain to the on_log event in user code which can then do whatever
	 * it wants to do with it.
	 * @param severity The log level from dpp::loglevel
	 * @param msg The log message to output
	 */
	virtual void log(dpp::loglevel severity, const std::string &msg) const;

	/**
	 * @brief Fires every second from the underlying socket I/O loop, used for sending heartbeats
	 * @throw dpp::exception if the socket needs to disconnect
	 */
	virtual void one_second_timer();

	/**
	 * @brief voice client is ready to stream audio.
	 * The voice client is considered ready if it has a secret key.
	 * 
	 * @return true if ready to stream audio
	 */
	bool is_ready();

	/**
	 * @brief Returns true if the voice client is connected to the websocket
	 * 
	 * @return True if connected
	 */
	bool is_connected();

	/**
	 * @brief Returns the connection time of the voice client
	 * 
	 * @return dpp::utility::uptime Detail of how long the voice client has been connected for
	 */
	dpp::utility::uptime get_uptime();

	/**
	 * @brief The time (in milliseconds) between each interval when parsing audio.
	 *
	 * @warning You should only change this if you know what you're doing. It is set to 500ms by default.
	 */
	uint16_t iteration_interval{500};

	/** Constructor takes shard id, max shards and token.
	 * @param _cluster The cluster which owns this voice connection, for related logging, REST requests etc
	 * @param _channel_id The channel id to identify the voice connection as
	 * @param _server_id The server id (guild id) to identify the voice connection as
	 * @param _token The voice session token to use for identifying to the websocket
	 * @param _session_id The voice session id to identify with
	 * @param _host The voice server hostname to connect to (hostname:port format)
	 * @param enable_dave Enable DAVE E2EE
	 * @throw dpp::voice_exception Sodium or Opus failed to initialise, or D++ is not compiled with voice support
	 * @warning DAVE E2EE is an EXPERIMENTAL feature!
	 */
	discord_voice_client(dpp::cluster* _cluster, snowflake _channel_id, snowflake _server_id, const std::string &_token, const std::string &_session_id, const std::string &_host, bool enable_dave = false);

	/**
	 * @brief Destroy the discord voice client object
	 */
	virtual ~discord_voice_client();

	/**
	 * @brief Handle JSON from the websocket.
	 * @param buffer The entire buffer content from the websocket client
	 * @return bool True if a frame has been handled
	 * @throw dpp::exception If there was an error processing the frame, or connection to UDP socket failed
	 */
	virtual bool handle_frame(const std::string &buffer, ws_opcode opcode);

	/**
	 * @brief Handle a websocket error.
	 * @param errorcode The error returned from the websocket
	 */
	virtual void error(uint32_t errorcode);

	/**
	 * @brief Start and monitor I/O loop
	 */
	void run();

	/**
	 * @brief Send raw audio to the voice channel.
	 * 
	 * You should send an audio packet of `send_audio_raw_max_length` (11520) bytes.
	 * Note that this function can be costly as it has to opus encode
	 * the PCM audio on the fly, and also encrypt it with libsodium.
	 * 
	 * @note Because this function encrypts and encodes packets before
	 * pushing them onto the output queue, if you have a complete stream
	 * ready to send and know its length it is advisable to call this
	 * method multiple times to enqueue the entire stream audio so that
	 * it is all encoded at once (unless you have set use_opus to false).
	 * **Constantly calling this from dpp::cluster::on_voice_buffer_send
	 * can, and will, eat a TON of cpu!**
	 * 
	 * @param audio_data Raw PCM audio data. Channels are interleaved,
	 * with each channel's amplitude being a 16 bit value.
	 * 
	 * @warning **The audio data needs to be 48000Hz signed 16 bit audio, otherwise, the audio will come through incorrectly!**
	 * 
	 * @param length The length of the audio data. The length should
	 * be a multiple of 4 (2x 16 bit stereo channels) with a maximum
	 * length of `send_audio_raw_max_length`, which is a complete opus
	 * frame at highest quality.
	 *
	 * Generally when you're streaming and you know there will be
	 * more packet to come you should always provide packet data with
	 * length of `send_audio_raw_max_length`.
	 * Silence packet will be appended if length is less than
	 * `send_audio_raw_max_length` as discord expects to receive such
	 * specific packet size. This can cause gaps in your stream resulting
	 * in distorted audio if you have more packet to send later on.
	 * 
	 * @return discord_voice_client& Reference to self
	 * 
	 * @throw dpp::voice_exception If data length is invalid or voice support not compiled into D++
	 */
	discord_voice_client& send_audio_raw(uint16_t* audio_data, const size_t length);

	/**
	 * @brief Send opus packets to the voice channel
	 * 
	 * Some containers such as .ogg may contain OPUS
	 * encoded data already. In this case, we don't need to encode the
	 * frames using opus here. We can bypass the codec, only applying 
	 * libsodium to the stream.
	 * 
	 * @param opus_packet Opus packets. Discord expects opus frames 
	 * to be encoded at 48000Hz
	 * 
	 * @param length The length of the audio data. 
	 * 
	 * @param duration Generally duration is 2.5, 5, 10, 20, 40 or 60
	 * if the timescale is 1000000 (1ms) 
	 * 
	 * @return discord_voice_client& Reference to self
	 * 
	 * @note It is your responsibility to ensure that packets of data 
	 * sent to send_audio are correctly repacketized for streaming, 
	 * e.g. that audio frames are not too large or contain
	 * an incorrect format. Discord will still expect the same frequency
	 * and bit width of audio and the same signedness.
	 * 
	 * @throw dpp::voice_exception If data length is invalid or voice support not compiled into D++
	 */
	discord_voice_client& send_audio_opus(uint8_t* opus_packet, const size_t length, uint64_t duration);

	/**
	 * @brief Send opus packets to the voice channel
	 * 
	 * Some containers such as .ogg may contain OPUS
	 * encoded data already. In this case, we don't need to encode the
	 * frames using opus here. We can bypass the codec, only applying 
	 * libsodium to the stream.
	 * 
	 * Duration is calculated internally
	 * 
	 * @param opus_packet Opus packets. Discord expects opus frames 
	 * to be encoded at 48000Hz
	 * 
	 * @param length The length of the audio data. 
	 * 
	 * @return discord_voice_client& Reference to self
	 * 
	 * @note It is your responsibility to ensure that packets of data 
	 * sent to send_audio are correctly repacketized for streaming, 
	 * e.g. that audio frames are not too large or contain
	 * an incorrect format. Discord will still expect the same frequency
	 * and bit width of audio and the same signedness.
	 * 
	 * @throw dpp::voice_exception If data length is invalid or voice support not compiled into D++
	 */
	discord_voice_client& send_audio_opus(uint8_t* opus_packet, const size_t length);

	/**
	 * @brief Send silence to the voice channel
	 * 
	 * @param duration How long to send silence for. With the standard
	 * timescale this is in milliseconds. Allowed values are 2.5,
	 * 5, 10, 20, 40 or 60 milliseconds.
	 * @return discord_voice_client& Reference to self
	 * @throw dpp::voice_exception if voice support is not compiled into D++
	 */
	discord_voice_client& send_silence(const uint64_t duration);

	/**
	 * @brief Sets the audio type that will be sent with send_audio_* methods.
	 *
	 * @see send_audio_type_t
	 */
	discord_voice_client& set_send_audio_type(send_audio_type_t type);

	/**
	 * @brief Set the timescale in nanoseconds.
	 * 
	 * @param new_timescale Timescale to set. This defaults to 1000000,
	 * which means 1 millisecond.
	 * @return discord_voice_client& Reference to self
	 * @throw dpp::voice_exception If data length is invalid or voice support not compiled into D++
	 */
	discord_voice_client& set_timescale(uint64_t new_timescale);

	/**
	 * @brief Get the current timescale, this will default to 1000000
	 * which means 1 millisecond.
	 * 
	 * @return uint64_t timescale in nanoseconds
	 */
	uint64_t get_timescale();

	/**
	 * @brief Mark the voice connection as 'speaking'.
	 * This sends a JSON message to the voice websocket which tells discord
	 * that the user is speaking. The library automatically calls this for you
	 * whenever you send audio.
	 * 
	 * @return discord_voice_client& Reference to self
	 */
	discord_voice_client& speak();

	/**
	 * @brief Pause sending of audio
	 * 
	 * @param pause True to pause, false to resume
	 * @return reference to self
	 */
	discord_voice_client& pause_audio(bool pause);

	/**
	 * @brief Immediately stop all audio.
	 * Clears the packet queue.
	 * @return reference to self
	 */
	discord_voice_client& stop_audio();

	/**
	 * @brief Change the iteration interval time.
	 *
	 * @param interval The time (in milliseconds) between each interval when parsing audio.
	 *
	 * @return Reference to self.
	 */
	discord_voice_client& set_iteration_interval(uint16_t interval);

	/**
	 * @brief Get the iteration interval time (in milliseconds).
	 *
	 * @return iteration_interval
	 */
	uint16_t get_iteration_interval();

	/**
	 * @brief Returns true if we are playing audio
	 * 
	 * @return true if audio is playing
	 */
	bool is_playing();

	/**
	 * @brief Get the number of seconds remaining
	 * of the audio output buffer
	 * 
	 * @return float number of seconds remaining 
	 */
	float get_secs_remaining();

	/**
	 * @brief Get the number of tracks remaining
	 * in the output buffer.
	 * This is calculated by the number of track
	 * markers plus one.
	 * @return uint32_t Number of tracks in the
	 * buffer
	 */
	uint32_t get_tracks_remaining();

	/**
	 * @brief Get the time remaining to send the
	 * audio output buffer in hours:minutes:seconds
	 * 
	 * @return dpp::utility::uptime length of buffer
	 */
	dpp::utility::uptime get_remaining();

	/**
	 * @brief Insert a track marker into the audio
	 * output buffer.
	 * A track marker is an arbitrary flag in the
	 * buffer contents that indicates the end of some
	 * block of audio of significance to the sender.
	 * This may be a song from a streaming site, or
	 * some voice audio/speech, a sound effect, or
	 * whatever you choose. You can later skip
	 * to the next marker using the
	 * dpp::discord_voice_client::skip_to_next_marker
	 * function.
	 * @param metadata Arbitrary information related to this
	 * track
	 * @return reference to self
	 */
	discord_voice_client& insert_marker(const std::string& metadata = "");

	/**
	 * @brief Skip tp the next track marker,
	 * previously inserted by using the
	 * dpp::discord_voice_client::insert_marker
	 * function. If there are no markers in the
	 * output buffer, then this skips to the end
	 * of the buffer and is equivalent to the
	 * dpp::discord_voice_client::stop_audio
	 * function.
	 * @note It is possible to use this function
	 * while the output stream is paused.
	 * @return reference to self
	 */
	discord_voice_client& skip_to_next_marker();

	/**
	 * @brief Get the metadata string associated with each inserted marker.
	 * 
	 * @return const std::vector<std::string>& list of metadata strings
	 */
	const std::vector<std::string> get_marker_metadata();

	/**
	 * @brief Returns true if the audio is paused.
	 * You can unpause with
	 * dpp::discord_voice_client::pause_audio.
	 * 
	 * @return true if paused
	 */
	bool is_paused();

	/**
	 * @brief Discord external IP detection.
	 * @return std::string Your external IP address
	 * @note This is a blocking operation that waits
	 * for a single packet from Discord's voice servers.
	 */
	std::string discover_ip();

	/**
	 * Returns true if end-to-end encryption is enabled
	 * for the active voice call (Discord Audio Visual
	 * Encryption, a.k.a. DAVE).
	 *
	 * @return True if end-to-end encrypted
	 */
	bool is_end_to_end_encrypted() const;

	/**
	 * Returns the privacy code for the end to end encryption
	 * scheme ("DAVE"). if end-to-end encryption is not active,
	 * or is not yet established, this will return an empty
	 * string.
	 *
	 * @return A sequence of six five-digit integers which
	 * can be matched against the Discord client, in the
	 * privacy tab for the properties of the voice call.
	 */
	std::string get_privacy_code() const;

	/**
	 * Returns the privacy code for a given user by id,
	 * if they are in the voice call, and enc-to-end encryption
	 * is enabled.
	 *
	 * @param user User ID to fetch the privacy code for
	 * @param callback Callback to call with the privacy code when
	 * the creation of the code is complete.
	 * @warning This call spawns a thread, as getting a user's
	 * privacy code is a CPU-intensive and memory-intensive operation
	 * which internally uses scrypt.
	 */
	void get_user_privacy_code(const dpp::snowflake user, privacy_code_callback_t callback) const;
};

} // namespace dpp

