module;

#include "modules.h"

#if !DPP_IMPORT_STD

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <functional>
#include <cstddef>
#include <variant>
#include <memory>
#include <string_view>
#include <type_traits>
#include <iomanip>
#include <locale>
#include <algorithm>
#include <sstream>
#include <iostream>
#include <charconv>

#endif

/**
 * @brief This is where we put the mostly standalone, mostly-not-discord-related stuff
 */
export module dpp.utility;

#if DPP_IMPORT_STD

import std;

#endif

import dpp.base;

#include "dpp/misc-enum.h"
#include "dpp/utility.h"
#include "dpp/unicode_emoji.h"
#include "dpp/stringops.h"
#include "dpp/once.h"
#include "dpp/colors.h"
