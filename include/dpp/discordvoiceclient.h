#pragma once
#include <errno.h>

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <io.h>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#else
#include <resolv.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <string>
#include <map>
#include <vector>
#include <dpp/json_fwd.hpp>
#include <dpp/wsclient.h>
#include <dpp/dispatcher.h>
#include <dpp/cluster.h>
#include <queue>
#include <thread>
#include <deque>
#include <mutex>

#ifdef HAVE_VOICE
#include <sodium.h>
#include <opus/opus.h>
#endif


using json = nlohmann::json;

namespace dpp {
	// Forward declaration
	class cluster;

/** @brief Implements a discord voice connection.
 * Each DiscordVoiceClient connects to one voice channel and derives from a websocket client.
 */
class DiscordVoiceClient : public WSClient
{
	/** Mutex for message queue */
	std::mutex queue_mutex;

	/** Queue of outbound messages */
	std::deque<std::string> message_queue;

	/** Thread this connection is executing on */
	std::thread* runner;

	/** Run shard loop under a thread */
	void ThreadRun();

	/** Last connect time of voice session */
	time_t connect_time;

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

	std::vector<std::string> outbuf;
	std::vector<std::string> inbuf;

#ifdef HAVE_VOICE
	OpusEncoder* encoder;
	OpusDecoder* decoder;
#endif

	/** File descriptor for UDP connection
	 */
	int fd;

	/** BSD Sockets address of voice server
	 */
	struct sockaddr_in servaddr;

public:

	fd_set readfds;
	fd_set writefds;

	/** Owning cluster */
	class dpp::cluster* creator;

	/* This needs to be static, we only initialise libsodium once per program start,
	* so initialising it on first use in a voice connection is best.
	*/
	static bool sodium_initialised;

	/** True when the thread is shutting down */
	bool terminating;

	/** Heartbeat interval for sending heartbeat keepalive */
	uint32_t heartbeat_interval;

	/** Last heartbeat */
	time_t last_heartbeat;

	/** Thread ID */
	std::thread::native_handle_type thread_id;

	/** Discord voice session token */
	std::string token;

	/** Discord voice session id */
	std::string sessionid;

	/** Server ID */
	snowflake server_id;

	/** Log a message to whatever log the user is using.
	 * The logged message is passed up the chain to the on_log event in user code which can then do whatever
	 * it wants to do with it.
	 * @param severity The log level from dpp::loglevel
	 * @param msg The log message to output
	 */
	void log(dpp::loglevel severity, const std::string &msg);

	/** Fires every second from the underlying socket I/O loop, used for sending heartbeats */
	virtual void OneSecondTimer();

	/**
	 * @brief Queue a message to be sent via the websocket
	 * 
	 * @param j The JSON data of the message to be sent
	 * @param to_front If set to true, will place the message at the front of the queue not the back
	 * (this is for urgent messages such as heartbeat, presence, so they can take precedence over
	 * chunk requests etc)
	 */
	void QueueMessage(const std::string &j, bool to_front = false);

	/**
	 * @brief Clear the outbound message queue
	 * 
	 */
	void ClearQueue();

	/**
	 * @brief Get the size of the outbound message queue
	 * 
	 * @return The size of the queue
	 */
	size_t GetQueueSize();

	/**
	 * @brief Returns true if the shard is connected
	 * 
	 * @return True if connected
	 */
	bool IsConnected();

	/**
	 * @brief Returns the connection time of the shard
	 * 
	 * @return dpp::utility::uptime Detail of how long the shard has been connected for
	 */
	dpp::utility::uptime Uptime();

	/** Constructor takes shard id, max shards and token.
	 * @param _cluster The owning cluster for this shard
	 * @param _server_id The server id to identify voice connection as
	 * @param _token The voice session token to use for identifying to the websocket
	 * @param _session_id The voice session id to identify with
	 * @param _host The voice server hostname to connect to (hostname:port format)
	 */
	DiscordVoiceClient(dpp::cluster* _cluster, snowflake _server_id, const std::string &_token, const std::string &_session_id, const std::string &_host);

	/** Destructor */
	virtual ~DiscordVoiceClient();

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

	int UDPSend(const char* data, size_t length);
	int UDPRecv(char* data, size_t max_length);

	int WantWrite();

	int WantRead();

	void WriteReady();

	void ReadReady();
};

};
