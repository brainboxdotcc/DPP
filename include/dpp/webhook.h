#pragma once
#include <dpp/discord.h>
#include <dpp/json_fwd.hpp>

namespace dpp {

enum webhook_type {
	w_incoming = 1,
	w_channel_follower = 2
};

class webhook : public managed {
public:
	uint8_t type;   		/* the type of the webhook */
	snowflake guild_id;     	/* Optional: the guild id this webhook is for */
	snowflake channel_id;   	/* the channel id this webhook is for */
	snowflake user_id;		/* Optional: the user this webhook was created by (not returned when getting a webhook with its token) */
	std::string name;		/* the default name of the webhook (may be empty) */
	std::string avatar;		/* the default avatar of the webhook (may be empty) */
	std::string token;		/* Optional: the secure token of the webhook (returned for Incoming Webhooks) */
	snowflake application_id;	/* the bot/OAuth2 application that created this webhook (may be empty) */
	std::string* image_data;


	webhook();
	~webhook();
	webhook& fill_from_json(nlohmann::json* j);
	std::string build_json(bool with_id = false) const;

	webhook& load_image(const std::string &image_blob, image_type type);
};

typedef std::unordered_map<snowflake, webhook> webhook_map;

};
