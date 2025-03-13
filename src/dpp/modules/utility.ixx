module;

#include "modules.h"
#include <dpp/version.h>

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
#include <set>
#include <thread>
#include <queue>
#include <mutex>
#include <shared_mutex>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <thread>
#include <functional>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <streambuf>
#include <array>

#endif

/**
 * @brief This is where we put the mostly standalone, mostly-not-discord-related stuff
 */
export module dpp:utility;

#if DPP_IMPORT_STD

import std;

#endif

import :base;

#include "dpp/misc-enum.h"
#include "dpp/utility.h"
#include "dpp/unicode_emoji.h"
#include "dpp/stringops.h"
#include "dpp/once.h"
#include "dpp/colors.h"
#include "dpp/thread_pool.h"
#include "dpp/timer.h"
