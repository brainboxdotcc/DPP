module;

#include "modules.h"
#include "dpp/dpp_fwd.h"

#if !DPP_IMPORT_STD

#include <string>
#include <exception>
#include <algorithm>
#include <charconv>
#include <type_traits>

#ifdef DPP_FORMATTERS
#include <format>
#endif

#endif

export module dpp.base;

#if DPP_IMPORT_STD

import std;

#endif

#include "dpp/json.h"
#include "dpp/exception.h"
#include "dpp/snowflake.h"
