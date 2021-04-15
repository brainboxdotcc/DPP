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

	/** Output buffer. Each string is a UDP packet.
	 * Generally these will be RTP.
	 */
	std::vector<std::string> outbuf;

	/** Input  buffer. Each string is a received UDP
	 * packet. These will usually be RTP.
	 */
	std::vector<std::string> inbuf;

#ifdef HAVE_VOICE
	/** libopus encoder
	 */
	OpusEncoder* encoder;

	/** libopus decoder
	 */
	OpusDecoder* decoder;

	/** libopus repacketizer
	 * (merges frames into one packet)
	 */
	OpusRepacketizer* repacketizer;
#endif

	/** File descriptor for UDP connection
	 */
	int fd;

	/** Socket address of voice server
	 */
	struct sockaddr_in servaddr;

	/** Secret key for encrypting voice.
	 * If it has been sent, this is non-null and points to a 
	 * sequence of exactly 32 bytes.
	 */
	uint8_t* secret_key;

	/** Sequence number of outbound audio. This is incremented
	 * once per frame sent.
	 */
	uint16_t sequence;

	/** Timestamp value used in outbound audio. Each packet
	 * has the timestamp value which is incremented to match
	 * how many frames are sent.
	 */
	uint32_t timestamp;

	/** This is set to true if we have started sending audio.
	 * When this moves from false to true, this causes the
	 * client to send the 'talking' notification to the websocket.
	 */
	bool sending;

	/** Encoding buffer for opus repacketizer and encode
	 */
	uint8_t encode_buffer[65536];

	/**
	 * @brief Send data to UDP socket immediately.
	 * 
	 * @param data data to send
	 * @param length length of data to send
	 * @return int bytes sent. Will return -1 if we cannot send
	 */
	int UDPSend(const char* data, size_t length);

	/**
	 * @brief Receieve data from UDP socket immediately.
	 * 
	 * @param data data to receive
	 * @param max_length size of data receiving buffer
	 * @return int bytes received. -1 if there is an error
	 * (e.g. EAGAIN)
	 */
	int UDPRecv(char* data, size_t max_length);

	/**
	 * @brief This hooks the SSLClient, returning the file
	 * descriptor if we want to send buffered data, or
	 * -1 if there is nothing to send
	 * 
	 * @return int file descriptor or -1
	 */
	int WantWrite();

	/**
	 * @brief This hooks the SSLClient, returning the file
	 * descriptor if we want to receive buffered data, or
	 * -1 if we are not wanting to receive
	 * 
	 * @return int file descriptor or -1
	 */
	int WantRead();

	/**
	 * @brief Called by SSLClient when the socket is ready
	 * for writing, at this point we pick the head item off
	 * the buffer and send it. So long as it doesnt error
	 * completely, we pop it off the head of the queue.
	 */
	void WriteReady();

	/**
	 * @brief Called by SSLClient when there is data to be
	 * read. At this point we insert that data into the
	 * input queue.
	 */
	void ReadReady();

	/**
	 * @brief Send data to the UDP socket, using the buffer.
	 * 
	 * @param packet packet data
	 * @param len length of packet
	 */
	void Send(const char* packet, size_t len);

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
	 * @brief Encode a byte buffer using opus codec.
	 * Multiple opus frames (2880 bytes each) will be encoded into one packet for sending.
	 * 
	 * @param input Input data as raw bytes of PCM data
	 * @param inDataSize Input data length
	 * @param output Output data as an opus encoded packet
	 * @param outDataSize Output data length, should be at least equal to the input size.
	 * Will be adjusted on return to the actual compressed data size.
	 * @return size_t The compressed data size that was encoded.
	 */
	size_t encode(uint8_t *input, size_t inDataSize, uint8_t *output, size_t &outDataSize);

public:

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

	/** Channel ID */
	snowflake channel_id;

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
	 * @brief voice client is ready to stream audio.
	 * The voice client is considered ready if it has a secret key.
	 * 
	 * @return true if ready to stream audio
	 */
	bool IsReady();

	/**
	 * @brief Returns true if the voice client is connected to the websocket
	 * 
	 * @return True if connected
	 */
	bool IsConnected();

	/**
	 * @brief Returns the connection time of the voice client
	 * 
	 * @return dpp::utility::uptime Detail of how long the voice client has been connected for
	 */
	dpp::utility::uptime Uptime();

	/** Constructor takes shard id, max shards and token.
	 * @param _cluster The owning cluster for this shard
	 * @param _server_id The server id to identify voice connection as
	 * @param _token The voice session token to use for identifying to the websocket
	 * @param _session_id The voice session id to identify with
	 * @param _host The voice server hostname to connect to (hostname:port format)
	 */
	DiscordVoiceClient(dpp::cluster* _cluster, snowflake _channel_id, snowflake _server_id, const std::string &_token, const std::string &_session_id, const std::string &_host);

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

	/**
	 * @brief Send audio to the voice channel.
	 * 
	 * You should send an audio packet of n11520 bytes.
	 * Note that this function can be costly as it has to opus encode
	 * the PCM audio on the fly, and also encrypt it with libsodium.
	 * 
	 * @note Because this function encrypts and encodes packets before
	 * pushing them onto the output queue, if you have a complete stream
	 * ready to send and know its length it is advisable to call this
	 * method multiple times to enqueue the entire stream audio so that
	 * it is all encoded at once. Constantly calling this from the
	 * dpp::on_voice_buffer_send callback can and will eat a TON of cpu!
	 * 
	 * @param audio_data Raw PCM audio data. Channels are interleaved,
	 * with each channel's amplitude being a 16 bit value.
	 * @param length The length of the audio data. The length should
	 * be a multiple of 4 (2x 16 bit stero channels) with a maximum
	 * length of 11520, which is a complete opus frame at highest
	 * quality.
	 * @param use_opus Some containers such as .ogg may contain OPUS
	 * encoded data already. In this case, we don't need to encode the
	 * frames using opus here. We can set use_opus to false and bypass the
	 * codec, only applying libsodium to the stream.
	 */
	void SendAudio(uint16_t* audio_data, const size_t length, bool use_opus = true);

	std::string DiscoverIP();
};

};
