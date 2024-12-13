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
#include <string>
#include <map>
#include <vector>
#include <dpp/json_fwd.h>
#include <dpp/wsclient.h>
#include <dpp/dispatcher.h>
#include <dpp/event.h>
#include <queue>
#include <thread>
#include <memory>
#include <deque>
#include <dpp/etf.h>
#include <mutex>
#include <shared_mutex>
#include <dpp/zlibcontext.h>

namespace dpp {

/**
 * @brief Discord API version for shard websockets and HTTPS API requests
 */
#define DISCORD_API_VERSION "10"

/**
 * @brief HTTPS Request base path for API calls
 */
#define API_PATH "/api/v" DISCORD_API_VERSION

/* Forward declarations */
class cluster;

/**
 * @brief How many seconds to wait between (re)connections. DO NOT change this.
 * It is mandated by the Discord API spec!
 */
constexpr time_t RECONNECT_INTERVAL = 5;

/**
 * @brief Represents different event opcodes sent and received on a shard websocket
 *
 * These are used internally to route frames.
 */
enum shard_frame_type : int {

	/**
	 * @brief An event was dispatched.
	 * @note Receive only
	 */
	ft_dispatch = 0,

	/**
	 * @brief Fired periodically by the client to keep the connection alive.
	 * @note Send/Receive
	 */
	ft_heartbeat = 1,

	/**
	 * @brief Starts a new session during the initial handshake.
	 * @note Send only
	 */
	ft_identify = 2,

	/**
	 * @brief Update the client's presence.
	 * @note Send only
	 */
	ft_presence = 3,

	/**
	 * @brief Used to join/leave or move between voice channels.
	 * @note Send only
	 */
	ft_voice_state_update = 4,

	/**
	 * @brief Resume a previous session that was disconnected.
	 * @note Send only
	 */
	ft_resume = 6,

	/**
	 * @brief You should attempt to reconnect and resume immediately.
	 * @note Receive only
	 */
	ft_reconnect = 7,

	/**
	 * @brief Request information about offline guild members in a large guild.
	 * @note Send only
	 */
	ft_request_guild_members = 8,

	/**
	 * @brief The session has been invalidated. You should reconnect and identify/resume accordingly.
	 * @note Receive only
	 */
	ft_invalid_session = 9,

	/**
	 * @brief Sent immediately after connecting, contains the heartbeat interval to use.
	 * @note Receive only
	 */
	ft_hello = 10,

	/**
	 * @brief Sent in response to receiving a heartbeat to acknowledge that it has been received.
	 * @note Receive only
	 */
	ft_heartbeat_ack = 11,

	/**
	 * @brief Request information about soundboard sounds in a set of guilds.
	 * @note Send only
	 */
	ft_request_soundboard_sounds = 31,
};

/**
 * @brief Represents a connection to a voice channel.
 * A client can only connect to one voice channel per guild at a time, so these are stored in a map
 * in the dpp::discord_client keyed by guild_id.
 */
class DPP_EXPORT voiceconn {
	/**
	 * @brief Owning dpp::discord_client instance
	 */
	class discord_client* creator;
public:
	/**
	 * @brief Voice Channel ID
	 */
	snowflake channel_id;

	/**
	 * @brief Websocket hostname for status
	 */
	std::string websocket_hostname;

	/**
	 * @brief Voice Voice session ID
	 */
	std::string session_id;

	/**
	 * @brief Voice websocket token
	 */
	std::string token;

	/**
	 * @brief voice websocket client
	 */
	class discord_voice_client* voiceclient;

	/**
	 * @brief True to enable DAVE E2EE
	 * @warning This is an EXPERIMENTAL feature!
	 */
	bool dave;

	/**
	 * @brief Construct a new voiceconn object
	 */
	voiceconn() = default;

	/**
	 * @brief Construct a new voiceconn object
	 * 
	 * @param o owner
	 * @param _channel_id voice channel id
	 * @param enable_dave True to enable DAVE E2EE
	 * @warn DAVE is an EXPERIMENTAL feature!
	 */
	voiceconn(class discord_client* o, snowflake _channel_id, bool enable_dave);

	/**
	 * @brief Destroy the voiceconn object
	 */
	~voiceconn();

	/**
	 * @brief return true if the connection is ready to connect
	 * (has hostname, token and session id)
	 * 
	 * @return true if ready to connect
	 */
	bool is_ready() const;
	
	/**
	 * @brief return true if the connection is active (websocket exists)
	 * 
	 * @return true if has an active websocket
	 */
	bool is_active() const;

	/**
	 * @brief Create websocket object and connect it.
	 * Needs hostname, token and session_id to be set or does nothing.
	 * 
	 * @param guild_id Guild to connect to the voice channel on
	 * @return reference to self
	 * @note It can spawn a thread to establish the connection, so this is NOT a synchronous blocking call!
	 * You shouldn't call this directly. Use a wrapper function instead. e.g. dpp::guild::connect_member_voice
	 */
	voiceconn& connect(snowflake guild_id);

	/**
	 * @brief Disconnect from the currently connected voice channel
	 * @return reference to self
	 */
	voiceconn& disconnect();
};

/** @brief Implements a discord client. Each discord_client connects to one shard and derives from a websocket client. */
class DPP_EXPORT discord_client : public websocket_client
{
protected:
	/**
	 * @brief Needed so that voice_state_update can call dpp::discord_client::disconnect_voice_internal
	 */
	friend class dpp::events::voice_state_update;

	/**
	 * @brief Needed so that guild_create can request member chunks if you have the correct intents
	 */
	friend class dpp::events::guild_create;

	/**
	 * @brief Needed to allow cluster::set_presence to use the ETF functions
	 */
	friend class dpp::cluster;

	/**
	 * @brief Disconnect from the connected voice channel on a guild
	 * 
	 * @param guild_id The guild who's voice channel you wish to disconnect from
	 * @param send_json True if we should send a json message confirming we are leaving the VC
	 * Should be set to false if we already receive this message in an event.
	 */
	void disconnect_voice_internal(snowflake guild_id, bool send_json = true);

	/**
	 * @brief Start connecting the websocket
	 *
	 * Called from the constructor, or during reconnection
	 */
	void start_connecting();

	/**
	 * @brief Stores the most recent ping message on this shard, which we check
	 * for to monitor latency
	 */
	std::string last_ping_message;

private:

	/**
	 * @brief Mutex for message queue
	 */
	std::shared_mutex queue_mutex;

	/**
	 * @brief Mutex for zlib pointer
	 */
	std::mutex zlib_mutex;

	/**
	 * @brief Queue of outbound messages
	 */
	std::deque<std::string> message_queue;

	/**
	 * @brief If true, stream compression is enabled
	 */
	bool compressed;

	/**
	 * @brief Decompressed string
	 */
	std::string decompressed;

	/**
	 * @brief This object contains the various zlib structs which
	 * are not usable by the user of the library directly. They
	 * are wrapped within this opaque object so that this header
	 * file does not bring in a dependency on zlib.h.
	 */
	std::unique_ptr<zlibcontext> zlib{};

	/**
	 * @brief Last connect time of cluster
	 */
	time_t connect_time;

	/**
	 * @brief Time last ping sent to websocket, in fractional seconds
	 */
	double ping_start;

	/**
	 * @brief ETF parser for when in ws_etf mode
	 */
	std::unique_ptr<etf_parser> etf;

	/**
	 * @brief Convert a JSON object to string.
	 * In JSON protocol mode, call json.dump(), and in ETF mode,
	 * call etf::build().
	 * 
	 * @param json nlohmann::json object to convert
	 * @return std::string string output in the correct format
	 */
	std::string jsonobj_to_string(const nlohmann::json& json);

	/**
	 * @brief Update the websocket hostname with the resume url
	 * from the last READY event
	 */
	void set_resume_hostname();

	/**
	 * @brief Clean up resources
	 */
	void cleanup();
public:
	/**
	 * @brief Owning cluster
	 */
	class dpp::cluster* creator;

	/**
	 * @brief Heartbeat interval for sending heartbeat keepalive
	 * @note value in milliseconds
	 */
	uint32_t heartbeat_interval;

	/**
	 * @brief Last heartbeat
	 */
	time_t last_heartbeat;

	/**
	 * @brief Shard ID of this client
	 */
	uint32_t shard_id;

	/**
	 * @brief Total number of shards
	 */
	uint32_t max_shards;

	/**
	 * @brief Last sequence number received, for resumes and pings
	 */
	uint64_t last_seq;

	/**
	 * @brief Discord bot token
	 */
	std::string token;

	/**
	 * @brief Privileged gateway intents
	 * @see dpp::intents
	 */
	uint32_t intents;

	/**
	 * @brief Discord session id
	 */
	std::string sessionid;

	/**
	 * @brief Mutex for voice connections map
	 */
	std::shared_mutex voice_mutex;

	/**
	 * @brief Resume count
	 */
	uint32_t resumes;

	/**
	 * @brief Reconnection count
	 */
	uint32_t reconnects;

	/**
	 * @brief Websocket latency in fractional seconds
	 */
	double websocket_ping;

	/**
	 * @brief True if READY or RESUMED has been received
	 */
	bool ready;

	/**
	 * @brief Last heartbeat ACK (opcode 11)
	 */
	time_t last_heartbeat_ack;

	/** 
	 * @brief Current websocket protocol, currently either ETF or JSON
	 */
	websocket_protocol_t protocol;

	/**
	 * @brief List of voice channels we are connecting to keyed by guild id
	 */
	std::unordered_map<snowflake, std::unique_ptr<voiceconn>> connecting_voice_channels;

	/**
	 * @brief The gateway address we reconnect to when we resume a session
	 */
	std::string resume_gateway_url;

	/**
	 * @brief Log a message to whatever log the user is using.
	 * The logged message is passed up the chain to the on_log event in user code which can then do whatever
	 * it wants to do with it.
	 * @param severity The log level from dpp::loglevel
	 * @param msg The log message to output
	 */
	virtual void log(dpp::loglevel severity, const std::string &msg) const override;

	/**
	 * @brief Handle an event (opcode 0)
	 * @param event Event name, e.g. MESSAGE_CREATE
	 * @param j JSON object for the event content
	 * @param raw Raw JSON event string
	 */
	virtual void handle_event(const std::string &event, json &j, const std::string &raw);

	/**
	 * @brief Get the Guild Count for this shard
	 * 
	 * @return uint64_t guild count
	 */
	uint64_t get_guild_count();

	/**
	 * @brief Get the Member Count for this shard
	 * 
	 * @return uint64_t member count
	 */
	uint64_t get_member_count();

	/**
	 * @brief Get the Channel Count for this shard
	 * 
	 * @return uint64_t channel count
	 */
	uint64_t get_channel_count();

	/**
	 * @brief Fires every second from the underlying socket I/O loop, used for sending heartbeats
	 * and any queued outbound websocket frames.
	 */
	virtual void one_second_timer() override;

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
	 * @return reference to self
	 */
	discord_client& clear_queue();

	/**
	 * @brief Get the size of the outbound message queue
	 * 
	 * @return The size of the queue
	 */
	size_t get_queue_size();

	/**
	 * @brief Returns true if the shard is connected
	 * 
	 * @return True if connected
	 */
	bool is_connected();

	/**
	 * @brief Returns the connection time of the shard
	 * 
	 * @return dpp::utility::uptime Detail of how long the shard has been connected for
	 */
	dpp::utility::uptime get_uptime();

	/**
	 * @brief Construct a new discord_client object
	 * 
	 * @param _cluster The owning cluster for this shard
	 * @param _shard_id The ID of the shard to start
	 * @param _max_shards The total number of shards across all clusters
	 * @param _token The bot token to use for identifying to the websocket
	 * @param intents Privileged intents to use, a bitmask of values from dpp::intents
	 * @param compressed True if the received data will be gzip compressed
	 * @param ws_protocol Websocket protocol to use for the connection, JSON or ETF
	 * 
	 * @throws std::bad_alloc Passed up to the caller if any internal objects fail to allocate, after cleanup has completed
	 */
	discord_client(dpp::cluster* _cluster, uint32_t _shard_id, uint32_t _max_shards, const std::string &_token, uint32_t intents = 0, bool compressed = true, websocket_protocol_t ws_protocol = ws_json);

	/**
	 * @brief Construct a discord_client object from another discord_client object
	 * Used when resuming, the url to connect to will be taken from the resume url of the
	 * other object, along with the seq number.
	 *
	 * @param old Previous connection to resume from
	 * @param sequence Sequence number of previous session
	 * @param session_id Session ID of previous session
	 */
	explicit discord_client(discord_client& old, uint64_t sequence, const std::string& session_id);

	/**
	 * @brief Destroy the discord client object
	 */
	virtual ~discord_client() = default;

	/**
	 * @brief Get decompressed total bytes received
	 *
	 * This will always return 0 if the connection is not compressed
	 * @return uint64_t compressed bytes received
	 */
	uint64_t get_decompressed_bytes_in();

	/**
	 * @brief Handle JSON from the websocket.
	 * @param buffer The entire buffer content from the websocket client
	 * @param opcode The type of frame, e.g. text or binary
	 * @returns True if a frame has been handled
	 */
	virtual bool handle_frame(const std::string &buffer, ws_opcode opcode) override;

	/**
	 * @brief Handle a websocket error.
	 * @param errorcode The error returned from the websocket
	 */
	virtual void error(uint32_t errorcode) override;

	/**
	 * @brief Start and monitor I/O loop.
	 */
	void run();

	/**
	 * @brief Called when the HTTP socket is closed
	 */
	virtual void on_disconnect() override;

	/**
	 * @brief Connect to a voice channel
	 * 
	 * @param guild_id Guild where the voice channel is
	 * @param channel_id Channel ID of the voice channel
	 * @param self_mute True if the bot should mute itself
	 * @param self_deaf True if the bot should deafen itself
	 * @param enable_dave True to enable DAVE E2EE - EXPERIMENTAL
	 * @return reference to self
	 * @note This is NOT a synchronous blocking call! The bot isn't instantly ready to send or listen for audio,
	 * as we have to wait for the connection to the voice server to be established!
	 * e.g. wait for dpp::cluster::on_voice_ready event, and then send the audio within that event.
	 */
	discord_client& connect_voice(snowflake guild_id, snowflake channel_id, bool self_mute = false, bool self_deaf = false, bool enable_dave = false);

	/**
	 * @brief Disconnect from the connected voice channel on a guild
	 * 
	 * @param guild_id The guild who's voice channel you wish to disconnect from
	 * @return reference to self
	 * @note This is NOT a synchronous blocking call! The bot isn't instantly disconnected.
	 */
	discord_client& disconnect_voice(snowflake guild_id);

	/**
	 * @brief Get the dpp::voiceconn object for a specific guild on this shard.
	 * 
	 * @param guild_id The guild ID to retrieve the voice connection for
	 * @return voiceconn* The voice connection for the guild, or nullptr if there is no
	 * voice connection to this guild.
	 */
	voiceconn* get_voice(snowflake guild_id);
};

}
