#pragma once

/**
 * WE ARE IN THE GLOBAL MODULE FRAGMENT
 * If you don't know what this means - don't change anything here.
 */

#include <dpp/export.h>

#undef DPP_EXPORT
#undef DPP_EXPORT_START
#undef DPP_EXPORT_END
#undef DPP_BUILD_MODULES

#define DPP_EXPORT export
#define DPP_EXPORT_START export {
#define DPP_EXPORT_END }
#define DPP_BUILD_MODULES 1

// Include our global C headers here
#include <cstdint>
#include <cstddef>
