#include <dpp/discord.h>

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

};
