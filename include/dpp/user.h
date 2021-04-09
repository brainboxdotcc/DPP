#pragma once

#include <dpp/json_fwd.hpp>

namespace dpp {

/** Various bitmask flags used to represent information about a dpp::user */
enum user_flags {
	/// User is a bot
	u_bot =			0b00000000000000000000001,
	/// User is a system user (Clyde!)
	u_system =		0b00000000000000000000010,
	/// User has multi-factor authentication enabled
	u_mfa_enabled =		0b00000000000000000000100,
	/// User is verified (verified email address)
	u_verified =		0b00000000000000000001000,
	/// User has full nitro
	u_nitro_full =		0b00000000000000000010000,
	/// User has nitro classic
	u_nitro_classic =	0b00000000000000000100000,
	/// User is discord staff
	u_discord_employee =	0b00000000000000001000000,
	/// User owns a partnered server
	u_partnered_owner =	0b00000000000000010000000,
	/// User is a member of hypesquad events
	u_hypesquad_events =	0b00000000000000100000000,
	/// User has BugHunter level 1
	u_bughunter_1 =		0b00000000000001000000000,
	/// User is a member of House Bravery
	u_house_bravery =	0b00000000000010000000000,
	/// User is a member of House Brilliance
	u_house_brilliance =	0b00000000000100000000000,
	/// User is a member of House Balance
	u_house_balanace =	0b00000000001000000000000,
	/// User is an early supporter
	u_early_supporter =	0b00000000010000000000000,
	/// User is a team user 
	u_team_user =		0b00000000100000000000000,
	/// User is has Bug Hunter level 2
	u_bughunter_2 =		0b00000000000000000000000,
	/// User is a verified bot
	u_verified_bot =	0b00000010000000000000000,
	/// User has the Early Verified Bot Developer badge
	u_verified_bot_dev =	0b00000100000000000000000,
};

/** Represents a user on discord. May or may not be a member of a dpp::guild.
 */
class user : public managed {
public:
	/** Discord username */
	std::string username;
	/** Discriminator (aka tag), 4 digits usually displayed with leading zeroes */
	uint16_t discriminator;
	/** Avatar hash */
	std::string avatar;
	/** Flags built from a bitmask of values in dpp::user_flags */
	uint32_t flags;

	/**
	 * @brief Construct a new user object
	 */
	user();

	/**
	 * @brief Destroy the user object
	 */
	~user();

	/** Fill this record from json.
	 * @param j The json to fill this record from
	 * @return Reference to self
	 */
	user& fill_from_json(nlohmann::json* j);

	/**
	 * @brief Get the avatar url of the user object
	 * 
	 * @return std::string avatar url
	 */
	std::string get_avatar_url();

	/**
	 * @brief User is a bot
	 * 
	 * @return True if the user is a bot
	 */
	bool is_bot();
	/**
	 * @brief User is a system user (Clyde)
	 * 
	 * @return true  if user is a system user
	 */
	bool is_system();
	/**
	 * @brief User has multi-factor authentication enabled
	 * 
	 * @return true if multi-factor is enabled
	 */
	bool is_mfa_enabled();
	/**
	 * @brief Return true if user has verified account
	 * 
	 * @return true if verified
	 */
	bool is_verified();
	/**
	 * @brief Return true if user has full nitro.
	 * This is mutually exclusive with full nitro.
	 * 
	 * @return true if user has full nitro
	 */
	bool has_nitro_full();
	/**
	 * @brief Return true if user has nitro classic.
	 * This is mutually exclusive with nitro classic.
	 * 
	 * @return true  if user has nitro classic
	 */
	bool has_nitro_classic();
	/**
	 * @brief Return true if user is a discord employee
	 * 
	 * @return true if user is discord staff
	 */
	bool is_discord_employee();
	/**
	 * @brief Return true if user owns a partnered server
	 * 
	 * @return true if user has partnered server
	 */
	bool is_partnered_owner();
	/**
	 * @brief Return true if user has hypesquad events
	 * 
	 * @return true if has hypesquad events
	 */
	bool has_hypesquad_events();
	/**
	 * @brief Return true if user has the bughunter level 1 badge
	 * 
	 * @return true if has bughunter level 1
	 */
	bool is_bughunter_1();
	/**
	 * @brief Return true if user is in house bravery
	 * 
	 * @return true if in house bravery
	 */
	bool is_house_bravery();
	/**
	 * @brief Return true if user is in house brilliance
	 * 
	 * @return true if in house brilliance
	 */
	bool is_house_brilliance();
	/**
	 * @brief Return true if user is in house balance
	 * 
	 * @return true if in house brilliance
	 */
	bool is_house_balanace();
	/**
	 * @brief Return true if user is an early supporter
	 * 
	 * @return true if early supporter
	 */
	bool is_early_supporter();
	/**
	 * @brief Return true if user is a team user
	 * 
	 * @return true if a team user
	 */
	bool is_team_user();
	/**
	 * @brief Return true if user has the bughunter level 2 badge
	 * 
	 * @return true if has bughunter level 2
	 */
	bool is_bughunter_2();
	/**
	 * @brief Return true if user has the verified bot badge
	 * 
	 * @return true if verified bot
	 */
	bool is_verified_bot();
	/**
	 * @brief Return true if user is an early verified bot developer
	 * 
	 * @return true if verified bot developer
	 */
	bool is_verified_bot_dev();
};

/** A group of users */
typedef std::unordered_map<snowflake, user> user_map;

};

