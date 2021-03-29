#pragma once

namespace dpp {

enum user_flags {
	u_bot =			0b00000000000000000000001,
	u_system =		0b00000000000000000000010,
	u_mfa_enabled =		0b00000000000000000000100,
	u_verified =		0b00000000000000000001000,
	u_nitro_full =		0b00000000000000000010000,
	u_nitro_classic =	0b00000000000000000100000,
	u_discord_employee =	0b00000000000000001000000,
	u_partnered_owner =	0b00000000000000010000000,
	u_hypesquad_events =	0b00000000000000100000000,
	u_bughunter_1 =		0b00000000000001000000000,
	u_house_bravery =	0b00000000000010000000000,
	u_house_brilliance =	0b00000000000100000000000,
	u_house_balanace =	0b00000000001000000000000,
	u_early_supporter =	0b00000000010000000000000,
	u_team_user =		0b00000000100000000000000,
	u_bughunter_2 =		0b00000000000000000000000,
	u_verified_bot =	0b00000010000000000000000,
	u_verified_bot_dev =	0b00000100000000000000000,
};

class user {
public:
	snowflake id;
	std::string username;
	uint16_t discriminator;
	std::string avatar;
	uint32_t flags;

	user();
	~user();

	bool is_bot();
	bool is_system();
	bool is_mfa_enabled();
	bool is_verified();
	bool has_nitro_full();
	bool has_nitro_classic();
	bool is_discord_employee();
	bool is_partnered_owner();
	bool has_hypesquad_events();
	bool is_bughunter_1();
	bool is_house_bravery();
	bool is_house_brilliance();
	bool is_house_balanace();
	bool is_early_supporter();
	bool is_team_user();
	bool is_bughunter_2();
	bool is_verified_bot();
	bool is_verified_bot_dev();
};

typedef std::unordered_map<snowflake, user*> user_map;

};
