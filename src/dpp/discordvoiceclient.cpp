#include <string>
#include <iostream>
#include <fstream>
#ifndef _WIN32
#include <unistd.h>
#endif
#include <dpp/discordvoiceclient.h>
#include <dpp/cache.h>
#include <dpp/cluster.h>
#include <thread>
#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <zlib.h>

namespace dpp {

bool DiscordVoiceClient::sodium_initialised = false;

DiscordVoiceClient::DiscordVoiceClient(dpp::cluster* _cluster, snowflake _server_id, const std::string &_token, const std::string &_session_id, const std::string &_host)
       : WSClient(_host.substr(0, _host.find(":")), _host.substr(_host.find(":") + 1, _host.length()), "/?v=4"),
	creator(_cluster),
	server_id(_server_id),
	token(_token),
	last_heartbeat(time(NULL)),
	heartbeat_interval(0),
	sessionid(_session_id),
	runner(nullptr),
	terminating(false)
{
	if (!DiscordVoiceClient::sodium_initialised) {
		if (sodium_init() < 0) {
			throw std::runtime_error("DiscordVoiceClient::DiscordVoiceClient; sodium_init() failed");
		}
		DiscordVoiceClient::sodium_initialised = true;
	}
	Connect();
}

DiscordVoiceClient::~DiscordVoiceClient()
{
	if (runner) {
		runner->join();
		delete runner;
	}
}

void DiscordVoiceClient::ThreadRun()
{
	do {
		SSLClient::ReadLoop();
		SSLClient::close();
		if (!terminating) {
			SSLClient::Connect();
			WSClient::Connect();
		}
	} while(!terminating);
}

void DiscordVoiceClient::Run()
{
	this->runner = new std::thread(&DiscordVoiceClient::ThreadRun, this);
	this->thread_id = runner->native_handle();
}

bool DiscordVoiceClient::HandleFrame(const std::string &data)
{
	log(dpp::ll_trace, fmt::format("R: {}", data));
	json j;
	
	try {
		j = json::parse(data);
	}
	catch (const std::exception &e) {
		log(dpp::ll_error, fmt::format("DiscordVoiceClient::HandleFrame {} [{}]", e.what(), data));
		return true;
	}

	if (j.find("op") != j.end()) {
		uint32_t op = j["op"];

		std::cout << data << "\n";

		switch (op) {
			/* Voice resume */
			case 9:
				log(ll_debug, "Voice connection resumed");
			break;
			/* Voice HELLO */
			case 8: {
				if (j.find("d") != j.end() && j["d"].find("heartbeat_interval") != j["d"].end() && !j["d"]["heartbeat_interval"].is_null()) {
					this->heartbeat_interval = j["d"]["heartbeat_interval"].get<uint32_t>();
				}

				if (modes.size()) {
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
				this->connect_time = time(NULL);
			}
			break;
			/* Voice ready */
			case 2:
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
			break;
		}
	}
	return true;
}

dpp::utility::uptime DiscordVoiceClient::Uptime()
{
	return dpp::utility::uptime(time(NULL) - connect_time);
}

bool DiscordVoiceClient::IsConnected()
{
	return (this->GetState() == CONNECTED);
}

void DiscordVoiceClient::Error(uint32_t errorcode)
{
	std::map<uint32_t, std::string> errortext = {
		{ 1000, "Socket shutdown" },
		{ 1001, "Client is leaving" },
		{ 1002, "Endpoint received a malformed frame" },
		{ 1003, "Endpoint received an unsupported frame" },
		{ 1004, "Reserved code" },
		{ 1005, "Expected close status, received none" },
		{ 1006, "No close code frame has been receieved" },
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
	log(dpp::ll_warning, fmt::format("OOF! Error from underlying websocket: {}: {}", errorcode, error));
}

void DiscordVoiceClient::log(dpp::loglevel severity, const std::string &msg)
{
	creator->log(severity, msg);
}

void DiscordVoiceClient::QueueMessage(const std::string &j, bool to_front)
{
	std::lock_guard<std::mutex> locker(queue_mutex);
	if (to_front) {
		message_queue.push_front(j);
	} else {
		message_queue.push_back(j);
	}
}

void DiscordVoiceClient::ClearQueue()
{
	std::lock_guard<std::mutex> locker(queue_mutex);
	message_queue.clear();
}

size_t DiscordVoiceClient::GetQueueSize()
{
	std::lock_guard<std::mutex> locker(queue_mutex);
	return message_queue.size();
}

void DiscordVoiceClient::OneSecondTimer()
{
	/* Rate limit outbound messages, 1 every odd second, 2 every even second */
	if (this->GetState() == CONNECTED) {
		for (int x = 0; x < (time(NULL) % 2) + 1; ++x) {
			std::lock_guard<std::mutex> locker(queue_mutex);
			if (message_queue.size()) {
				std::string message = message_queue.front();
				message_queue.pop_front();
				this->write(message);
			}
		}

		if (this->heartbeat_interval) {
			/* Check if we're due to emit a heartbeat */
			if (time(NULL) > last_heartbeat + ((heartbeat_interval / 1000.0) * 0.75)) {
				log(dpp::ll_debug, "Voice: Emit heartbeat");
				QueueMessage(json({{"op", 3}, {"d", 12345678}}).dump(), true);
				last_heartbeat = time(NULL);
			}
		}
	}
}


};