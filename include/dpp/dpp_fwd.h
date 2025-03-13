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

struct socket_events;

class discord_client;
class discord_voice_client;

class channel;
class guild_member;

}
