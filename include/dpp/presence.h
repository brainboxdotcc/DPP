#pragma once

#include <dpp/discord.h>
#include <dpp/json_fwd.hpp>

namespace dpp {

enum presence_flags {
	p_desktop_online	=	0b00000001,
	p_desktop_dnd		=	0b00000010,
	p_desktop_idle		=	0b00000011,
	p_web_online		=	0b00000100,
	p_web_dnd		=	0b00001000,
	p_web_idle		=	0b00001100,
	p_mobile_online		=	0b00010000,
	p_mobile_dnd		=	0b00100000,
	p_mobile_idle		=	0b00110000,
	p_status_online		=	0b01000000,
	p_status_dnd		=	0b10000000,
	p_status_idle		=	0b11000000
};

enum presence_status : uint8_t {
	ps_offline	=	0,
	ps_online	=	1,
	ps_dnd		=	2,
	ps_idle		=	3
};

#define PF_SHIFT_DESKTOP	0
#define PF_SHIFT_WEB		2
#define PF_SHIFT_MOBILE		4
#define PF_SHIFT_MAIN		6

#define PF_STATUS_MASK		0b00000011

#define PF_CLEAR_DESKTOP	0b11111100
#define PF_CLEAR_WEB		0b11110011
#define PF_CLEAR_MOBILE		0b11001111
#define PF_CLEAR_STATUS		0b00111111

enum activity_type {
	at_game		=	0,
	at_listening	=	1,
	at_streaming	=	2,
	at_custom	=	3,
	at_competing	=	4
};

enum activity_flags {
	af_instance	= 0b00000001,
	af_join		= 0b00000010,
	af_spectate	= 0b00000100,
	af_join_request	= 0b00001000,
	af_sync		= 0b00010000,
	af_play		= 0b00100000
};

class activity {
public:
	std::string name;
	std::string state;
	std::string url;
	uint8_t type;
	time_t created_at;
	time_t start;
	time_t end;
	snowflake application_id;
	uint8_t flags;
};

class presence {
public:
	snowflake	user_id;
	snowflake       guild_id;
	uint8_t		flags;
	std::vector<activity> activities;

	presence();
	~presence();
	presence& fill_from_json(nlohmann::json* j);
	std::string build_json() const;

	presence_status desktop_status() const;
	presence_status web_status() const;
	presence_status mobile_status() const;
	presence_status status() const;
};

/** A container of presences */
typedef std::unordered_map<std::string, presence> presence_map;

};
