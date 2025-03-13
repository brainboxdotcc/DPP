module;

#include "modules.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>
#include <fcntl.h>
#include <csignal>
#include <cstring>

#if !DPP_IMPORT_STD

#include <string>
#include <exception>
#include <algorithm>
#include <charconv>
#include <type_traits>
#include <string>
#include <map>
#include <vector>
#include <queue>
#include <thread>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <future>
#include <functional>
#include <chrono>
#include <set>

#ifdef DPP_FORMATTERS
#include <format>
#endif

#endif

#include "dpp/json.h"

export module dpp:voice;

#if DPP_IMPORT_STD

import std;

#endif

import :base;
import :utility;
import :socketengine;
import :discord;
import :rest;

#include "dpp/discordvoiceclient.h"

