#pragma once

#include <dpp/discord.h>
#include <dpp/json_fwd.hpp>

namespace dpp {

enum voicestate_flags {
	vs_deaf		=	0b00000001,
	vs_mute		=	0b00000010,
	vs_self_mute	=	0b00000100,
	vs_self_deaf	=	0b00001000,
	vs_self_stream	=	0b00010000,
	vs_self_video	=	0b00100000,
	vs_supress	=	0b01000000
};

class voicestate {
public:
	snowflake       guild_id;       /* Optional: the guild id this voice state is for */
	snowflake       channel_id;     /* the channel id this user is connected to (may be empty) */
	snowflake       user_id;        /* the user id this voice state is for */
	std::string     session_id;     /* the session id for this voice state */
	uint8_t		flags;

	voicestate();
	~voicestate();
	voicestate& fill_from_json(nlohmann::json* j);
	std::string build_json() const;

	bool is_deaf() const;
	bool is_mute() const;
	bool is_self_mute() const;
	bool is_self_deaf() const;
	bool self_stream() const;
	bool self_video() const;
	bool is_supressed() const;
};

/** A container of voicestates */
typedef std::unordered_map<std::string, voicestate> voicestate_map;

};
