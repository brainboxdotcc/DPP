#include <dpp/discord.h>
#include <dpp/dispatcher.h>

namespace dpp {

event_dispatch_t::event_dispatch_t(const std::string &raw) : raw_event(raw)
{
}

log_t::log_t(const std::string &raw) : event_dispatch_t(raw)
{
}

voice_state_update_t::voice_state_update_t(const std::string &raw) : event_dispatch_t(raw)
{
}

interaction_create_t::interaction_create_t(const std::string &raw) : event_dispatch_t(raw)
{
}

guild_delete_t::guild_delete_t(const std::string &raw) : event_dispatch_t(raw)
{
}

channel_delete_t::channel_delete_t(const std::string &raw) : event_dispatch_t(raw)
{
}

channel_update_t::channel_update_t(const std::string &raw) : event_dispatch_t(raw)
{
}

ready_t::ready_t(const std::string &raw) : event_dispatch_t(raw)
{
}

message_delete_t::message_delete_t(const std::string &raw) : event_dispatch_t(raw)
{
}

application_command_delete_t::application_command_delete_t(const std::string &raw) : event_dispatch_t(raw)
{
}

application_command_create_t::application_command_create_t(const std::string &raw) : event_dispatch_t(raw)
{
}

resumed_t::resumed_t(const std::string &raw) : event_dispatch_t(raw)
{
}

guild_role_create_t::guild_role_create_t(const std::string &raw) : event_dispatch_t(raw)
{
}

typing_start_t::typing_start_t(const std::string &raw) : event_dispatch_t(raw)
{
}

message_reaction_add_t::message_reaction_add_t(const std::string &raw) : event_dispatch_t(raw)
{
}

message_reaction_remove_t::message_reaction_remove_t(const std::string &raw) : event_dispatch_t(raw)
{
}

guild_create_t::guild_create_t(const std::string &raw) : event_dispatch_t(raw)
{
}

channel_create_t::channel_create_t(const std::string &raw) : event_dispatch_t(raw)
{
}

message_reaction_remove_emoji_t::message_reaction_remove_emoji_t(const std::string &raw) : event_dispatch_t(raw)
{
}

message_delete_bulk_t::message_delete_bulk_t(const std::string &raw) : event_dispatch_t(raw)
{
}

guild_role_update_t::guild_role_update_t(const std::string &raw) : event_dispatch_t(raw)
{
}

guild_role_delete_t::guild_role_delete_t(const std::string &raw) : event_dispatch_t(raw)
{
}

channel_pins_update_t::channel_pins_update_t(const std::string &raw) : event_dispatch_t(raw)
{
}

message_reaction_remove_all_t::message_reaction_remove_all_t(const std::string &raw) : event_dispatch_t(raw)
{
}

voice_server_update_t::voice_server_update_t(const std::string &raw) : event_dispatch_t(raw)
{
}

guild_emojis_update_t::guild_emojis_update_t(const std::string &raw) : event_dispatch_t(raw)
{
}

presence_update_t::presence_update_t(const std::string &raw) : event_dispatch_t(raw)
{
}

webhooks_update_t::webhooks_update_t(const std::string &raw) : event_dispatch_t(raw)
{
}

guild_member_add_t::guild_member_add_t(const std::string &raw) : event_dispatch_t(raw)
{
}

invite_delete_t::invite_delete_t(const std::string &raw) : event_dispatch_t(raw)
{
}

guild_update_t::guild_update_t(const std::string &raw) : event_dispatch_t(raw)
{
}

guild_integrations_update_t::guild_integrations_update_t(const std::string &raw) : event_dispatch_t(raw)
{
}

guild_member_update_t::guild_member_update_t(const std::string &raw) : event_dispatch_t(raw)
{
}

application_command_update_t::application_command_update_t(const std::string &raw) : event_dispatch_t(raw)
{
}

invite_create_t::invite_create_t(const std::string &raw) : event_dispatch_t(raw)
{
}

message_update_t::message_update_t(const std::string &raw) : event_dispatch_t(raw)
{
}

user_update_t::user_update_t(const std::string &raw) : event_dispatch_t(raw)
{
}

message_create_t::message_create_t(const std::string &raw) : event_dispatch_t(raw)
{
}

guild_ban_add_t::guild_ban_add_t(const std::string &raw) : event_dispatch_t(raw)
{
}

guild_ban_remove_t::guild_ban_remove_t(const std::string &raw) : event_dispatch_t(raw)
{
}

integration_create_t::integration_create_t(const std::string &raw) : event_dispatch_t(raw)
{
}

integration_update_t::integration_update_t(const std::string &raw) : event_dispatch_t(raw)
{
}

integration_delete_t::integration_delete_t(const std::string &raw) : event_dispatch_t(raw)
{
}

guild_member_remove_t::guild_member_remove_t(const std::string &raw) : event_dispatch_t(raw)
{
}

guild_members_chunk_t::guild_members_chunk_t(const std::string &raw) : event_dispatch_t(raw)
{
}

voice_buffer_send_t::voice_buffer_send_t(const std::string &raw) : event_dispatch_t(raw)
{
}

voice_user_talking_t::voice_user_talking_t(const std::string &raw) : event_dispatch_t(raw)
{
}

voice_ready_t::voice_ready_t(const std::string &raw) : event_dispatch_t(raw)
{
}

};
