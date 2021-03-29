#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <map>

namespace dpp {
	typedef int64_t snowflake;
};

class managed {
public:
	~managed() = default;
};

#include <dpp/role.h>
#include <dpp/user.h>
#include <dpp/channel.h>
#include <dpp/guild.h>
#include <dpp/intents.h>
