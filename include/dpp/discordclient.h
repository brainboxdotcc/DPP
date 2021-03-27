#pragma once
#include <string>
#include <map>
#include <vector>
#include <variant>
#include <spdlog/spdlog.h>
#include <dpp/wsclient.h>

/* Implements a discord client. Each DiscordClient connects to one shard and derives from a websocket client. */
class DiscordClient : public WSClient
{
	/* Heartbeat interval for sending heartbeat keepalive */
	uint32_t heartbeat_interval;

	/* Last heartbeat */
	time_t last_heartbeat;

	/* Shard ID of this client */
	uint32_t shard_id;

	/* Total number of shards */
	uint32_t max_shards;

	/* Last sequence number received, for resumes and pings */
	uint64_t last_seq;

	/* Discord bot token */
	std::string token;

	/* Discord session id */
	std::string sessionid;

	/* Opaque logger */
	class spdlog::logger* logger;

protected:

	virtual void OneSecondTimer();
public:
	/* Constructor takes shard id, max shards and token */
        DiscordClient(uint32_t _shard_id, uint32_t _max_shards, const std::string &_token, class spdlog::logger* _logger = nullptr);

	/* Destructor */
        virtual ~DiscordClient();

	/* Handle JSON from the websocket */
	virtual bool HandleFrame(const std::string &buffer);

	/* Handle a websocket error */
	virtual void Error(uint32_t errorcode);

	/* Start and monitor I/O loop */
	void Run();
};

