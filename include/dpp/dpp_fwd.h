#pragma once

#include <dpp/export.h>

#if !DPP_BUILD_MODULES
#include <dpp/json_fwd.h>
#endif

DPP_EXPORT namespace dpp
{

class cluster;
struct confirmation_callback_t;
template <typename T> class event_router_t;

class channel;

struct socket_events;

}
