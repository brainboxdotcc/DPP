#include <map>
#include <dpp/discord.h>
#include <dpp/cluster.h>
#include <dpp/discordclient.h>
#include <dpp/message.h>
#include <spdlog/spdlog.h>
#include <chrono>

namespace dpp {

cluster::cluster(const std::string &_token, uint32_t _intents, uint32_t _shards, uint32_t _cluster_id, uint32_t _maxclusters, spdlog::logger* _log)
	: token(_token), intents(_intents), numshards(_shards), cluster_id(_cluster_id), maxclusters(_maxclusters), log(_log)
{
	rest = new request_queue(this);
}

cluster::~cluster()
{
	delete rest;
}

void cluster::start() {
	/* Start up all shards */
	for (uint32_t s = 0; s < numshards; ++s) {
		/* Filter out shards that arent part of the current cluster, if the bot is clustered */
		if (s % maxclusters == cluster_id) {
			/* TODO: DiscordClient should spawn a thread in its Run() */
			this->shards[s] = new DiscordClient(this, s, numshards, token, intents, log);
			this->shards[s]->Run();
			std::this_thread::sleep_for(std::chrono::seconds(5));
		}
	}
}

void cluster::post_rest(const std::string &endpoint, const std::string &parameters, http_method method, const std::string &postdata, http_completion_event callback) {
	/* NOTE: This is not a memory leak! The request_queue will free the http_request once it reaches the end of its lifecycle */
	rest->post_request(new http_request(endpoint, parameters, callback, postdata, method));
}

void cluster::message_create(const message &m, http_completion_event callback) {
	this->post_rest("/api/channels", std::to_string(m.channel_id) + "/messages", m_post, m.build_json(), callback);
}

void cluster::message_edit(const message &m, http_completion_event callback) {
	this->post_rest("/api/channels", std::to_string(m.channel_id) + "/messages/" + std::to_string(m.id), m_patch, m.build_json(true), callback);
}

void cluster::message_delete(snowflake message_id, snowflake channel_id, http_completion_event callback) {
	this->post_rest("/api/channels", std::to_string(channel_id) + "/messages/" + std::to_string(message_id), m_delete, "", callback);
}

void cluster::channel_create(const class channel &c, http_completion_event callback) {
	this->post_rest("/api/channels", "", m_post, c.build_json(), callback);
}

void cluster::channel_edit(const class channel &c, http_completion_event callback) {
	this->post_rest("/api/channels", std::to_string(c.id), m_patch, c.build_json(true), callback);
}

void cluster::channel_delete(snowflake channel_id, http_completion_event callback) {
	this->post_rest("/api/channels", std::to_string(channel_id), m_delete, "", callback);
}

void cluster::guild_create(const class guild &g, http_completion_event callback) {
	this->post_rest("/api/guilds", "", m_post, g.build_json(), callback);
}

void cluster::guild_edit(const class guild &g, http_completion_event callback) {
	this->post_rest("/api/guilds", std::to_string(g.id), m_patch, g.build_json(true), callback);
}

void cluster::guild_delete(snowflake guild_id, http_completion_event callback) {
	this->post_rest("/api/guilds", std::to_string(guild_id), m_delete, "", callback);
}

void cluster::role_create(const class role &r, http_completion_event callback) {
	this->post_rest("/api/channels", std::to_string(r.guild_id) + "/roles", m_post, r.build_json(), callback);
}

void cluster::role_edit(const class role &r, http_completion_event callback) {
	json j = r.build_json(true);
	auto p = j.find("position");
	if (p != j.end()) {
		j.erase(p);
	}
	this->post_rest("/api/guilds", std::to_string(r.guild_id) + "/roles/" + std::to_string(r.id) , m_patch, j, callback);
}

void cluster::role_edit_position(const class role &r, http_completion_event callback) {
	json j({ {"id", r.id}, {"position", r.position}  });
	this->post_rest("/api/guilds", std::to_string(r.guild_id) + "/roles/" + std::to_string(r.id), m_patch, j, callback);
}



void cluster::role_delete(snowflake guild_id, snowflake role_id, http_completion_event callback) {
	this->post_rest("/api/guilds", std::to_string(guild_id) + "/roles/" + std::to_string(role_id), m_delete, "", callback);
}



void cluster::on_voice_state_update (std::function<void(const voice_state_update_t& _event)> _voice_state_update) {
	this->dispatch.voice_state_update = _voice_state_update; 
}

void cluster::on_interaction_create (std::function<void(const interaction_create_t& _event)> _interaction_create) {
	this->dispatch.interaction_create = _interaction_create; 
}

void cluster::on_guild_delete (std::function<void(const guild_delete_t& _event)> _guild_delete) {
	this->dispatch.guild_delete = _guild_delete; 
}

void cluster::on_channel_delete (std::function<void(const channel_delete_t& _event)> _channel_delete) {
	this->dispatch.channel_delete = _channel_delete; 
}

void cluster::on_channel_update (std::function<void(const channel_update_t& _event)> _channel_update) {
	this->dispatch.channel_update = _channel_update; 
}

void cluster::on_ready (std::function<void(const ready_t& _event)> _ready) {
	this->dispatch.ready = _ready; 
}

void cluster::on_message_delete (std::function<void(const message_delete_t& _event)> _message_delete) {
	this->dispatch.message_delete = _message_delete; 
}

void cluster::on_application_command_delete (std::function<void(const application_command_delete_t& _event)> _application_command_delete) {
	this->dispatch.application_command_delete = _application_command_delete; 
}

void cluster::on_guild_member_remove (std::function<void(const guild_member_remove_t& _event)> _guild_member_remove) {
	this->dispatch.guild_member_remove = _guild_member_remove; 
}

void cluster::on_application_command_create (std::function<void(const application_command_create_t& _event)> _application_command_create) {
	this->dispatch.application_command_create = _application_command_create; 
}

void cluster::on_resumed (std::function<void(const resumed_t& _event)> _resumed) {
	this->dispatch.resumed = _resumed; 
}

void cluster::on_guild_role_create (std::function<void(const guild_role_create_t& _event)> _guild_role_create) {
	this->dispatch.guild_role_create = _guild_role_create; 
}

void cluster::on_typing_start (std::function<void(const typing_start_t& _event)> _typing_start) {
	this->dispatch.typing_start = _typing_start; 
}

void cluster::on_message_reaction_add (std::function<void(const message_reaction_add_t& _event)> _message_reaction_add) {
	this->dispatch.message_reaction_add = _message_reaction_add; 
}

void cluster::on_guild_members_chunk (std::function<void(const guild_members_chunk_t& _event)> _guild_members_chunk) {
	this->dispatch.guild_members_chunk = _guild_members_chunk; 
}

void cluster::on_message_reaction_remove (std::function<void(const message_reaction_remove_t& _event)> _message_reaction_remove) {
	this->dispatch.message_reaction_remove = _message_reaction_remove; 
}

void cluster::on_guild_create (std::function<void(const guild_create_t& _event)> _guild_create) {
	this->dispatch.guild_create = _guild_create; 
}

void cluster::on_channel_create (std::function<void(const channel_create_t& _event)> _channel_create) {
	this->dispatch.channel_create = _channel_create; 
}

void cluster::on_message_reaction_remove_emoji (std::function<void(const message_reaction_remove_emoji_t& _event)> _message_reaction_remove_emoji) {
	this->dispatch.message_reaction_remove_emoji = _message_reaction_remove_emoji; 
}

void cluster::on_message_delete_bulk (std::function<void(const message_delete_bulk_t& _event)> _message_delete_bulk) {
	this->dispatch.message_delete_bulk = _message_delete_bulk; 
}

void cluster::on_guild_role_update (std::function<void(const guild_role_update_t& _event)> _guild_role_update) {
	this->dispatch.guild_role_update = _guild_role_update; 
}

void cluster::on_guild_role_delete (std::function<void(const guild_role_delete_t& _event)> _guild_role_delete) {
	this->dispatch.guild_role_delete = _guild_role_delete; 
}

void cluster::on_channel_pins_update (std::function<void(const channel_pins_update_t& _event)> _channel_pins_update) {
	this->dispatch.channel_pins_update = _channel_pins_update; 
}

void cluster::on_message_reaction_remove_all (std::function<void(const message_reaction_remove_all_t& _event)> _message_reaction_remove_all) {
	this->dispatch.message_reaction_remove_all = _message_reaction_remove_all; 
}

void cluster::on_voice_server_update (std::function<void(const voice_server_update_t& _event)> _voice_server_update) {
	this->dispatch.voice_server_update = _voice_server_update; 
}

void cluster::on_guild_emojis_update (std::function<void(const guild_emojis_update_t& _event)> _guild_emojis_update) {
	this->dispatch.guild_emojis_update = _guild_emojis_update; 
}

void cluster::on_presence_update (std::function<void(const presence_update_t& _event)> _presence_update) {
	this->dispatch.presence_update = _presence_update; 
}

void cluster::on_webhooks_update (std::function<void(const webhooks_update_t& _event)> _webhooks_update) {
	this->dispatch.webhooks_update = _webhooks_update; 
}

void cluster::on_guild_member_add (std::function<void(const guild_member_add_t& _event)> _guild_member_add) {
	this->dispatch.guild_member_add = _guild_member_add; 
}

void cluster::on_invite_delete (std::function<void(const invite_delete_t& _event)> _invite_delete) {
	this->dispatch.invite_delete = _invite_delete; 
}

void cluster::on_guild_update (std::function<void(const guild_update_t& _event)> _guild_update) {
	this->dispatch.guild_update = _guild_update; 
}

void cluster::on_guild_integrations_update (std::function<void(const guild_integrations_update_t& _event)> _guild_integrations_update) {
	this->dispatch.guild_integrations_update = _guild_integrations_update; 
}

void cluster::on_guild_member_update (std::function<void(const guild_member_update_t& _event)> _guild_member_update) {
	this->dispatch.guild_member_update = _guild_member_update; 
}

void cluster::on_application_command_update (std::function<void(const application_command_update_t& _event)> _application_command_update) {
	this->dispatch.application_command_update = _application_command_update; 
}

void cluster::on_invite_create (std::function<void(const invite_create_t& _event)> _invite_create) {
	this->dispatch.invite_create = _invite_create; 
}

void cluster::on_message_update (std::function<void(const message_update_t& _event)> _message_update) {
	this->dispatch.message_update = _message_update; 
}

void cluster::on_user_update (std::function<void(const user_update_t& _event)> _user_update) {
	this->dispatch.user_update = _user_update; 
}

void cluster::on_message_create (std::function<void(const message_create_t& _event)> _message_create) {
	this->dispatch.message_create = _message_create; 
}

void cluster::on_guild_ban_add (std::function<void(const guild_ban_add_t& _event)> _guild_ban_add) {
	this->dispatch.guild_ban_add = _guild_ban_add; 
}

void cluster::on_integration_create (std::function<void(const integration_create_t& _event)> _integration_create) {
	this->dispatch.integration_create = _integration_create;
}

void cluster::on_integration_update (std::function<void(const integration_update_t& _event)> _integration_update) {
	this->dispatch.integration_update = _integration_update;
}

void cluster::on_integration_delete (std::function<void(const integration_delete_t& _event)> _integration_delete) {
	this->dispatch.integration_delete = _integration_delete;
}


};
