#include <dpp/discord.h>
#include <dpp/discordevents.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::map<uint32_t, dpp::user_flags> usermap = {
	{ 1 << 0,       dpp::u_discord_employee },
	{ 1 << 1,       dpp::u_partnered_owner },
	{ 1 << 2,       dpp::u_hypesquad_events },
	{ 1 << 3,       dpp::u_bughunter_1 },
	{ 1 << 6,       dpp::u_house_bravery },
	{ 1 << 7,       dpp::u_house_brilliance },
	{ 1 << 8,       dpp::u_house_balanace },
	{ 1 << 9,       dpp::u_early_supporter },
	{ 1 << 10,      dpp::u_team_user },
	{ 1 << 14,      dpp::u_bughunter_2 },
	{ 1 << 16,      dpp::u_verified_bot },
	{ 1 << 17,      dpp::u_verified_bot_dev }
};

namespace dpp {

user::user() :
	id(0),
	discriminator(0),
	flags(0)
{
}

user::~user()
{
}

bool user::is_bot() {
	 return this->flags & u_bot; 
}

bool user::is_system() {
	 return this->flags & u_system; 
}

bool user::is_mfa_enabled() {
	 return this->flags & u_mfa_enabled; 
}

bool user::is_verified() {
	 return this->flags & u_verified; 
}

bool user::has_nitro_full() {
	 return this->flags & u_nitro_full; 
}

bool user::has_nitro_classic() {
	 return this->flags & u_nitro_classic; 
}

bool user::is_discord_employee() {
	 return this->flags & u_discord_employee; 
}

bool user::is_partnered_owner() {
	 return this->flags & u_partnered_owner; 
}

bool user::has_hypesquad_events() {
	 return this->flags & u_hypesquad_events; 
}

bool user::is_bughunter_1() {
	 return this->flags & u_bughunter_1; 
}

bool user::is_house_bravery() {
	 return this->flags & u_house_bravery; 
}

bool user::is_house_brilliance() {
	 return this->flags & u_house_brilliance; 
}

bool user::is_house_balanace() {
	 return this->flags & u_house_balanace; 
}

bool user::is_early_supporter() {
	 return this->flags & u_early_supporter; 
}

bool user::is_team_user() {
	 return this->flags & u_team_user; 
}

bool user::is_bughunter_2() {
	 return this->flags & u_bughunter_2; 
}

bool user::is_verified_bot() {
	 return this->flags & u_verified_bot; 
}

bool user::is_verified_bot_dev() {
	 return this->flags & u_verified_bot_dev; 
}

void user::fill_from_json(json* j) {
	this->id = SnowflakeNotNull(j, "id");
	this->username = StringNotNull(j, "username");
	this->avatar = StringNotNull(j, "avatar");
	this->discriminator = SnowflakeNotNull(j, "discriminator");
	this->flags |= BoolNotNull(j, "bot") ? dpp::u_bot : 0;
	this->flags |= BoolNotNull(j, "system") ? dpp::u_system : 0;
	this->flags |= BoolNotNull(j, "mfa_enabled") ? dpp::u_mfa_enabled : 0;
	this->flags |= BoolNotNull(j, "verified") ? dpp::u_verified : 0;
	this->flags |= BoolNotNull(j, "premium_type") == 1 ? dpp::u_nitro_classic : 0;
	this->flags |= BoolNotNull(j, "premium_type") == 2 ? dpp::u_nitro_full : 0;
	uint32_t flags = Int32NotNull(j, "flags");
	for (auto & flag : usermap) {
		if (flags & flag.first) {
			this->flags |= flag.second;
		}
	}
}

};
