#include <dpp/discord.h>

namespace dpp {

channel::channel() :
	id(0),
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

};
