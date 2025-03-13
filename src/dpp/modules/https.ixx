module;

#include "modules.h"
#include "dpp/version.h"

#if !DPP_IMPORT_STD

#include <string>
#include <exception>
#include <algorithm>
#include <charconv>
#include <type_traits>
#include <unordered_map>
#include <string>
#include <queue>
#include <map>
#include <shared_mutex>
#include <memory>
#include <vector>
#include <functional>
#include <atomic>
#include <list>

#ifdef DPP_FORMATTERS
#include <format>
#endif

#endif

#include "dpp/json.h"

export module dpp:https;

#if DPP_IMPORT_STD

import std;

#endif

import :base;
import :utility;
import :socketengine;

#include "dpp/zlibcontext.h"
#include "dpp/sslclient.h"
#include "dpp/wsclient.h"
#include "dpp/httpsclient.h"
#include "dpp/queues.h"
