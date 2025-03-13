module;

#include "modules.h"

#include <nlohmann/json.hpp>

export module dpp;

export import :base;
export import :utility;
export import :coro;
export import :socketengine;
export import :https;
export import :discord;
