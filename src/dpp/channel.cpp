#include <dpp/discord.h>
#include <dpp/discordevents.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace dpp {

channel::channel() :
	managed(),
	flags(0),
	guild_id(0),
	position(0),
	last_message_id(0),
	user_limit(0),
	rate_limit_per_user(0),
	owner_id(0),
	parent_id(0),
	last_pin_timestamp(0)
{
}

channel::~channel()
{
}

bool channel::is_nsfw() {
	return flags & dpp::c_nsfw;
}

bool channel::is_text_channel() {
	return flags & dpp::c_text;
}

bool channel::is_dm() {
	return flags & dpp::c_dm;
}

bool channel::is_voice_channel() {
	return flags & dpp::c_voice;
}

bool channel::is_group_dm() {
	return (flags & (dpp::c_dm | dpp::c_group)) == (dpp::c_dm | dpp::c_group);
}

bool channel::is_category() {
	return flags & dpp::c_category;
}

bool channel::is_news_channel() {
	return flags & dpp::c_news;
}

bool channel::is_store_channel() {
	return flags & dpp::c_store;
}

void channel::fill_from_json(json* j) {
	this->id = SnowflakeNotNull(j, "id");
	this->guild_id = SnowflakeNotNull(j, "guild_id");
	this->position = Int16NotNull(j, "position");
	this->name = StringNotNull(j, "name");
	this->topic = StringNotNull(j, "topic");
	this->last_message_id = SnowflakeNotNull(j, "last_message_id");
	this->user_limit = Int32NotNull(j, "user_limit");
	this->rate_limit_per_user = Int16NotNull(j, "rate_limit_per_user");
	this->owner_id = SnowflakeNotNull(j, "owner_id");
	this->parent_id = SnowflakeNotNull(j, "parent_id");
	//this->last_pin_timestamp
	uint8_t type = Int8NotNull(j, "type");
	this->flags |= BoolNotNull(j, "nsfw") ? dpp::c_nsfw : 0;
	this->flags |= (type == GUILD_TEXT) ? dpp::c_text : 0;
	this->flags |= (type == GUILD_VOICE) ? dpp::c_voice : 0;
	this->flags |= (type == DM) ? dpp::c_dm : 0;
	this->flags |= (type == GROUP_DM) ? (dpp::c_group | dpp::c_dm) : 0;
	this->flags |= (type == GUILD_CATEGORY) ? dpp::c_category : 0;
	this->flags |= (type == GUILD_NEWS) ? dpp::c_news : 0;
	this->flags |= (type == GUILD_STORE) ? dpp::c_store : 0;
}

std::string channel::build_json(bool with_id) {
	json j;
	if (with_id) {
		j["id"] = std::to_string(id);
	}
	j["guild_id"] = std::to_string(guild_id);
	j["position"] = position;
	j["name"] = name;
	j["topic"] = topic;
	if (is_voice_channel()) {
		j["user_limit"] = user_limit; 
		j["rate_limit_per_user"] = rate_limit_per_user;
	}
	if (!is_dm()) {
		j["parent_id"] = parent_id;
		if (is_text_channel()) {
			j["type"] = GUILD_TEXT;
		} else if (is_voice_channel()) {
			j["type"] = GUILD_VOICE;
		} else if (is_category()) {
			j["type"] = GUILD_CATEGORY;
		} else if (is_news_channel()) {
			j["type"] = GUILD_NEWS;
		} else if (is_store_channel()) {
			j["type"] = GUILD_STORE;
		}
		j["nsfw"] = is_nsfw();
	} else {
		if (is_group_dm()) {
			j["type"] = GROUP_DM;
		} else  {
			j["type"] = DM;
		}
	}

	return j.dump();
}

};
