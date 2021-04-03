#include <dpp/discord.h>
#include <dpp/discordevents.h>
#include <dpp/stringops.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace dpp {

role::role() :
	managed(),
	colour(0),
	position(0),
	permissions(0),
	flags(0),
	integration_id(0),
	bot_id(0)
{
}

role::~role()
{
}

void role::fill_from_json(nlohmann::json* j)
{
	this->id = SnowflakeNotNull(j, "id");
	this->colour = Int32NotNull(j, "color");
	this->position = Int8NotNull(j, "position");
	this->permissions = Int32NotNull(j, "permissions");
	this->flags |= BoolNotNull(j, "hoist") ? dpp::r_hoist : 0;
	this->flags |= BoolNotNull(j, "managed") ? dpp::r_managed : 0;
	this->flags |= BoolNotNull(j, "mentionable") ? dpp::r_mentionable : 0;
	if (j->find("tags") != j->end()) {
		auto t = (*j)["tags"];
		this->flags |= BoolNotNull(&t, "premium_subscriber") ? dpp::r_premium_subscriber : 0;
		this->bot_id = SnowflakeNotNull(&t, "bot_id");
		this->integration_id = SnowflakeNotNull(&t, "integration_id");
	}
}

std::string role::build_json(bool with_id) {
	json j;

	if (with_id) {
		j["id"] = std::to_string(id);
	}
	if (colour) {
		j["color"] = colour;
	}
	j["position"] = position;
	j["permissions"] = permissions;
	j["hoist"] = is_hoisted();
	j["mentionable"] = is_mentionable();

	return j.dump();
}

bool role::is_hoisted() {
	return this->flags & dpp::r_hoist;
}

bool role::is_mentionable() {
	return this->flags & dpp::r_mentionable;
}

bool role::is_managed() {
	return this->flags & dpp::r_managed;
}

bool role::has_create_instant_invite() {
	return ((this->permissions & p_administrator) | (this->permissions & p_create_instant_invite));
}

bool role::has_kick_members() {
	return ((this->permissions & p_administrator) | (this->permissions & p_kick_members));
}

bool role::has_ban_members() {
	return ((this->permissions & p_administrator) | (this->permissions & p_ban_members));
}

bool role::has_administrator() {
	return (this->permissions & p_administrator);
}

bool role::has_manage_channels() {
	return ((this->permissions & p_administrator) | (this->permissions & p_manage_channels));
}

bool role::has_manage_guild() {
	return ((this->permissions & p_administrator) | (this->permissions & p_manage_guild));
}

bool role::has_add_reactions() {
	return ((this->permissions & p_administrator) | (this->permissions & p_add_reactions));
}

bool role::has_view_audit_log() {
	return ((this->permissions & p_administrator) | (this->permissions & p_view_audit_log));
}

bool role::has_priority_speaker() {
	return ((this->permissions & p_administrator) | (this->permissions & p_priority_speaker));
}

bool role::has_stream() {
	return ((this->permissions & p_administrator) | (this->permissions & p_stream));
}

bool role::has_view_channel() {
	return ((this->permissions & p_administrator) | (this->permissions & p_view_channel));
}

bool role::has_send_messages() {
	return ((this->permissions & p_administrator) | (this->permissions & p_send_messages));
}

bool role::has_send_tts_messages() {
	return ((this->permissions & p_administrator) | (this->permissions & p_send_tts_messages));
}

bool role::has_manage_messages() {
	return ((this->permissions & p_administrator) | (this->permissions & p_manage_messages));
}

bool role::has_embed_links() {
	return ((this->permissions & p_administrator) | (this->permissions & p_embed_links));
}

bool role::has_attach_files() {
	return ((this->permissions & p_administrator) | (this->permissions & p_attach_files));
}

bool role::has_read_message_history() {
	return ((this->permissions & p_administrator) | (this->permissions & p_read_message_history));
}

bool role::has_mention_everyone() {
	return ((this->permissions & p_administrator) | (this->permissions & p_mention_everyone));
}

bool role::has_use_external_emojis() {
	return ((this->permissions & p_administrator) | (this->permissions & p_use_external_emojis));
}

bool role::has_view_guild_insights() {
	return ((this->permissions & p_administrator) | (this->permissions & p_view_guild_insights));
}

bool role::has_connect() {
	return ((this->permissions & p_administrator) | (this->permissions & p_connect));
}

bool role::has_speak() {
	return ((this->permissions & p_administrator) | (this->permissions & p_speak));
}

bool role::has_mute_members() {
	return ((this->permissions & p_administrator) | (this->permissions & p_mute_members));
}

bool role::has_deafen_members() {
	return ((this->permissions & p_administrator) | (this->permissions & p_deafen_members));
}

bool role::has_move_members() {
	return ((this->permissions & p_administrator) | (this->permissions & p_move_members));
}

bool role::has_use_vad() {
	return ((this->permissions & p_administrator) | (this->permissions & p_use_vad));
}

bool role::has_change_nickname() {
	return ((this->permissions & p_administrator) | (this->permissions & p_change_nickname));
}

bool role::has_manage_nicknames() {
	return ((this->permissions & p_administrator) | (this->permissions & p_manage_nicknames));
}

bool role::has_manage_roles() {
	return ((this->permissions & p_administrator) | (this->permissions & p_manage_roles));
}

bool role::has_manage_webhooks() {
	return ((this->permissions & p_administrator) | (this->permissions & p_manage_webhooks));
}

bool role::has_manage_emojis() {
	return ((this->permissions & p_administrator) | (this->permissions & p_manage_emojis));
}

};
