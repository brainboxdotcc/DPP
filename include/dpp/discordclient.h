#pragma once
#include <string>
#include <map>
#include <vector>
#include <dpp/json_fwd.hpp>
#include <dpp/wsclient.h>
#include <dpp/dispatcher.h>
#include <dpp/cluster.h>
#include <queue>
#include <thread>
#include <zlib.h>

using json = nlohmann::json;

namespace dpp {
	// Forward declaration
	class cluster;
};

/** @brief Implements a discord client. Each DiscordClient connects to one shard and derives from a websocket client. */
class DiscordClient : public WSClient
{
	/** Queue of guild ids we are requesting member chunks for */
	std::queue<uint64_t> chunk_queue;

	/** Thread this shard is executing on */
	std::thread* runner;

	/** Run shard loop under a thread */
	void ThreadRun();

	/** If true, stream compression is enabled */
	bool compressed;

	/** ZLib decompression buffer */
	unsigned char* decomp_buffer;

	/** Decompressed string */
	std::string decompressed;

	/** Frame decompression stream */
	z_stream d_stream;

	/** Total decompressed received bytes */
	uint64_t decompressed_total;

	void SetupZLib();
	void EndZLib();

public:
	/** Owning cluster */
	class dpp::cluster* creator;

	/** Heartbeat interval for sending heartbeat keepalive */
	uint32_t heartbeat_interval;

	/** Last heartbeat */
	time_t last_heartbeat;

	/** Shard ID of this client */
	uint32_t shard_id;

	/** Total number of shards */
	uint32_t max_shards;

	/** Thread ID */
	std::thread::native_handle_type thread_id;

	/** Last sequence number received, for resumes and pings */
	uint64_t last_seq;

	/** Discord bot token */
	std::string token;

	/** Privileged gateway intents */
	uint32_t intents;

	/** Discord session id */
	std::string sessionid;

	/** Log a message to whatever log the user is using.
	 * The logged message is passed up the chain to the on_log event in user code which can then do whatever
	 * it wants to do with it.
	 * @param severity The log level from dpp::loglevel
	 * @param msg The log message to output
	 */
	void log(dpp::loglevel severity, const std::string &msg);

	/** Handle an event (opcode 0)
	 * @param event Event name, e.g. MESSAGE_CREATE
	 * @param j JSON object for the event content
	 * @param raw Raw JSON event string
	 */
	virtual void HandleEvent(const std::string &event, json &j, const std::string &raw);

	/** Fires every second from the underlying socket I/O loop, used for sending heartbeats */
	virtual void OneSecondTimer();

	/** Constructor takes shard id, max shards and token.
	 * @param _cluster The owning cluster for this shard
	 * @param _shard_id The ID of the shard to start
	 * @param _max_shards The total number of shards across all clusters
	 * @param _token The bot token to use for identifying to the websocket
	 * @param intents Privileged intents to use, a bitmask of values from dpp::intents
	 * @param compressed True if the received data will be gzip compressed
	 */
	DiscordClient(dpp::cluster* _cluster, uint32_t _shard_id, uint32_t _max_shards, const std::string &_token, uint32_t intents = 0, bool compressed = true);

	/** Destructor */
	virtual ~DiscordClient();

	/** Get decompressed total bytes received */
	uint64_t GetDeompressedBytesIn();

	/** Handle JSON from the websocket.
	 * @param buffer The entire buffer content from the websocket client
	 * @returns True if a frame has been handled
	 */
	virtual bool HandleFrame(const std::string &buffer);

	/** Handle a websocket error.
	 * @param errorcode The error returned from the websocket
	 */
	virtual void Error(uint32_t errorcode);

	/** Start and monitor I/O loop */
	void Run();

	/** Add a guild to the chunk queue, to request its guild member chunks on a timer
	 * (must be on a timer because the guild member chunk requests are rate limited).
	 * @param id Guild to add to the chunk queue
	 */
	void add_chunk_queue(uint64_t id);

};

