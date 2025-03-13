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

#endif

#include "dpp/json.h"

export module dpp:discord;

import :base;
import :utility;
import :https;

// /!\ There is a delicate order here. Proceed with caution especially further down the list
#include "dpp/managed.h"
#include "dpp/json_interface.h"
#include "dpp/intents.h"
#include "dpp/sku.h"
#include "dpp/voiceregion.h"
#include "dpp/emoji.h"
#include "dpp/ban.h"
#include "dpp/permissions.h"
#include "dpp/automod.h"
#include "dpp/auditlog.h"
#include "dpp/user.h"
#include "dpp/voicestate.h"
#include "dpp/entitlement.h"
#include "dpp/stage_instance.h"
#include "dpp/dtemplate.h"
#include "dpp/prune.h"
// Past this point, headers need at least one of the above
#include "dpp/presence.h"
#include "dpp/channel.h"
#include "dpp/guild.h"
#include "dpp/role.h"
#include "dpp/integration.h"
#include "dpp/scheduled_event.h"
// Past this point we need user, guild, channel
#include "dpp/message.h"
#include "dpp/thread.h"
#include "dpp/invite.h"
#include "dpp/webhook.h"
#include "dpp/application.h"
#include "dpp/appcommand.h" // This one basically needs everything

/*


*/