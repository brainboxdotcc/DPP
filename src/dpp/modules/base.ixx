module;

#include "modules.h"

#if !DPP_IMPORT_STD

#include <string>
#include <exception>
#include <algorithm>
#include <charconv>
#include <type_traits>

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#ifdef DPP_FORMATTERS
#include <format>
#endif

#endif

export module dpp:base;

#if DPP_IMPORT_STD

import std;

#endif

extern "C++" {

#ifdef DPP_USE_EXTERNAL_JSON
	#include <nlohmann/json_fwd.hpp>
#else
	#include <dpp/nlohmann/json_fwd.hpp>
#endif

}

#include "dpp/dpp_fwd.h"
#include "dpp/json.h"
#include "dpp/exception.h"
#include "dpp/snowflake.h"
