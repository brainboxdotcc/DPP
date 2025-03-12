#pragma once

/**
 * WE ARE IN THE GLOBAL MODULE FRAGMENT
 * If you don't know what this means - don't change anything here.
 */

#include <dpp/export.h>

#undef DPP_EXPORT
#undef DPP_EXPORT_START
#undef DPP_EXPORT_END

#define DPP_EXPORT export
#define DPP_EXPORT_START export {
#define DPP_EXPORT_END }

// Include our global C headers here
#include <cstdint>
#include <cstddef>

#ifdef DPP_USE_EXTERNAL_JSON
	#include <nlohmann/json.hpp>
#else
	#include <dpp/nlohmann/json.hpp>
#endif

#include "dpp/dpp_fwd.h"
#include "dpp/json.h"
