#pragma once

#include <dpp/json_fwd.hpp>

namespace dpp {

enum integration_type {
	i_twitch,
	i_youtube,
	i_discord
};

enum integration_flags {
	if_enabled =     0b00000001,
	if_syncing =     0b00000010,
	if_emoticons =   0b00000100,
	if_revoked =     0b00001000,
	if_expire_kick = 0b00010000,
};

struct integration_app {
	snowflake id;
	std::string name;
	std::string icon;
	std::string description;
	std::string summary;
	user* bot;
};

/** Represents an integration within a dpp::guild */
class integration : public managed {
public:
	/** Integration name */
	std::string name;
	/** Integration type */
	integration_type type;
	/** Integration flags from dpp::integration_flags */
	uint8_t flags;
	/** Role id */
	snowflake role_id;
	/** User id */
	snowflake user_id;
	/** Expiry grace period */
	uint32_t expire_grace_period;
	/** Sync time */
	time_t synced_at;
	/** Subscriber count */
	uint32_t subscriber_count;
	/* Account id */
	std::string account_id;
	/* Account name */
	std::string account_name;
	/* Integration application */
	integration_app app;

	/** Default constructor */
	integration();

	/** Default destructor */
	~integration();

	/** Fill this integration from json.
	 * @param j The json data
	 */
	integration& fill_from_json(nlohmann::json* j);

	/** Build a json string from this object.
	 */
	std::string build_json() const;

	bool emoticons_enabled() const;
	bool is_enabled() const;
	bool is_syncing() const;
	bool is_revoked() const;
	bool expiry_kicks_user() const;
};

/** A group of integrations */
typedef std::unordered_map<snowflake, integration> integration_map;

};

