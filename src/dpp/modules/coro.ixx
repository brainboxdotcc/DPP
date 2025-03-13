module;

#include "modules.h"

#if !DPP_IMPORT_STD

#include <coroutine>
#include <utility>
#include <memory>
#include <variant>
#include <utility>
#include <type_traits>
#include <functional>
#include <atomic>
#include <mutex>
#include <optional>
#include <utility>
#include <type_traits>
#include <exception>
#include <atomic>
#include <condition_variable>
#include <optional>
#include <type_traits>
#include <exception>
#include <utility>
#include <type_traits>
#include <utility>
#include <type_traits>
#include <optional>
#include <functional>
#include <mutex>
#include <exception>
#include <atomic>
#include <iostream> // std::cerr in final_suspend
#include <atomic>
#include <array>
#include <memory>
#include <limits>
#include <optional>

#endif

export module dpp:coro;

#if DPP_IMPORT_STD

import std;

#endif

import :base;
import :utility;

#include "dpp/coro/coro.h"
#include "dpp/coro/job.h"
#include "dpp/coro/awaitable.h"
#include "dpp/coro/async.h"
#include "dpp/coro/coroutine.h"
#include "dpp/coro/task.h"
#include "dpp/coro/when_any.h"
