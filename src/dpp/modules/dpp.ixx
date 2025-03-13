module;

#include "modules.h"
#include <dpp/version.h>

#include <nlohmann/json.hpp>

#if !DPP_IMPORT_STD
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
#include <variant>
#include <iostream>
#endif

#ifdef _WIN32
	#include <stdio.h>
	#include <stdlib.h>
	#define popen _popen
	#define pclose _pclose
#endif

#ifdef HAVE_PRCTL
	#include <sys/prctl.h>
#endif

export module dpp;

export import :base;
export import :utility;
export import :coro;
export import :socketengine;
export import :https;
export import :discord;
export import :rest;
export import :voice;
