module;

#include "modules.h"

#include <cstring>

#if !DPP_IMPORT_STD

#include <string>
#include <map>
#include <variant>
#include <thread>
#include <algorithm>
#include <iostream>
#include <shared_mutex>
#include <unordered_map>
#include <vector>
#include <functional>
#include <variant>
#include <exception>
#include <algorithm>
#include <string>
#include <string>
#include <map>
#include <variant>
#include <thread>
#include <algorithm>
#include <iostream>
#include <shared_mutex>
#include <cstring>
#include <deque>
#include <string>
#include <map>
#include <vector>
#include <queue>
#include <thread>
#include <memory>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <coroutine>

#endif

#include <nlohmann/json.hpp>

export module dpp:rest;

import :base;
import :utility;
import :https;
import :discord;
import :coro;

#include "dpp/dispatcher.h"
#include "dpp/etf.h"
#include "dpp/event.h"
#include "dpp/event_router.h"
#include "dpp/discordclient.h"
#include "dpp/cache.h"
#include "dpp/restresults.h"
#include "dpp/cluster.h"
#include "dpp/restrequest.h"
#include "dpp/timed_listener.h"
#include "dpp/collector.h"
