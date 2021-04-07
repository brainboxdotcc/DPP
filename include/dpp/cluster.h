#pragma once

#include <string>
#include <map>
#include <variant>
#include <dpp/discord.h>
#include <dpp/dispatcher.h>
#include <dpp/json_fwd.hpp>
#include <spdlog/fwd.h>
#include <dpp/discordclient.h>
#include <dpp/queues.h>

using  json = nlohmann::json;

namespace dpp {

struct gateway {
	std::string url;
	uint32_t shards;
	uint32_t session_start_total;
	uint32_t session_start_remaining;
	uint32_t session_start_reset_after;
	uint32_t session_start_max_concurrency;
	gateway(nlohmann::json* j);
};

struct confirmation {
	bool success;
};

typedef std::variant<
		confirmation,
		message,
		message_map,
		user,
		user_map,
		guild_member,
		guild_member_map,
		channel,
		channel_map,
		guild,
		guild_map,
		role,
		role_map,
		invite,
		invite_map,
		dtemplate,
		dtemplate_map,
		emoji,
		emoji_map,
		ban,
		ban_map,
		voiceregion,
		voiceregion_map,
		integration,
		integration_map,
		webhook,
		webhook_map,
		prune,
		guild_widget,
		gateway
	> confirmable_t;

struct confirmation_callback_t {
	std::string type;
	http_request_completion_t http_info;
	confirmable_t value;

	confirmation_callback_t() = default;
	confirmation_callback_t(const std::string &_type, const confirmable_t& _value, const http_request_completion_t& _http);
};

typedef std::function<void(const confirmation_callback_t&)> command_completion_event_t;
typedef std::function<void(json&, const http_request_completion_t&)> json_encode_t;

/** The cluster class represents a group of shards and a command queue for sending and 
 * receiving commands from discord via HTTP. You should usually instantiate a cluster object
 * at the very least to make use of the library.
 */
class cluster {
	/** queue system for commands sent to Discord, and any replies */
	request_queue* rest;

	/** Accepts result from /gateway/bot REST API call and populates numshards with it */
	void auto_shard(const confirmation_callback_t &shardinfo);
public:
	/** Current bot token for all shards on this cluster and all commands sent via HTTP */
	std::string token;

	/** Current bitmask of gateway intents */
	uint32_t intents;

	/** Total number of shards across all clusters */
	uint32_t numshards;

	/** ID of this cluster, between 0 and MAXCLUSTERS-1 inclusive */
	uint32_t cluster_id;

	/** Total number of clusters that are active */
	uint32_t maxclusters;

	/** Optional spdlog::logger log object */
	spdlog::logger* log;

	/** Routes events from Discord back to user program code via std::functions */
	dpp::dispatcher dispatch;

	/** Active shards on this cluster. Shard IDs may have gaps between if there 
	 * are multiple clusters.
	 */
	std::map<uint32_t, class DiscordClient*> shards;

	/** Constructor for creating a cluster. All but the token are optional.
	 * @param token The bot token to use for all HTTP commands and websocket connections
	 * @param intents A bitmask of dpd::intents values for all shards on this cluster. This is required to be sent for all bots with over 100 servers.
	 * @param shards The total number of shards on this bot. If there are multiple clusters, then (shards / clusters) actual shards will run on this cluster.
	 * If you omit this value, the library will attempt to query the Discord API for the correct number of shards to start.
	 * @param cluster_id The ID of this cluster, should be between 0 and MAXCLUSTERS-1
	 * @param maxclusters The total number of clusters that are active, which may be on seperate processes or even separate machines.
	 * @param log An optional spdlog::logger object for logging details about the cluster
	 */
	cluster(const std::string &token, uint32_t intents = 0, uint32_t shards = 0, uint32_t cluster_id = 0, uint32_t maxclusters = 1, spdlog::logger* log = nullptr);

	/** Destructor */
	~cluster();

	/** Set or change current logger */
	void set_logger(spdlog::logger* log);

	/** Start the cluster, connecting all its shards.
	 * Returns once all shards are connected.
	 */
	void start();

	/* Functions for attaching to event handlers */

	/** Called for VOICE_STATE_UPDATE */
	void on_voice_state_update (std::function<void(const voice_state_update_t& _event)> _voice_state_update);

	/** Called for ON_INTERACTION_CREATE */
	void on_interaction_create (std::function<void(const interaction_create_t& _event)> _interaction_create);
	void on_guild_delete (std::function<void(const guild_delete_t& _event)> _guild_delete);
	void on_channel_delete (std::function<void(const channel_delete_t& _event)> _channel_delete);
	void on_channel_update (std::function<void(const channel_update_t& _event)> _channel_update);
	void on_ready (std::function<void(const ready_t& _event)> _ready);
	void on_message_delete (std::function<void(const message_delete_t& _event)> _message_delete);
	void on_application_command_delete (std::function<void(const application_command_delete_t& _event)> _application_command_delete);
	void on_guild_member_remove (std::function<void(const guild_member_remove_t& _event)> _guild_member_remove);
	void on_application_command_create (std::function<void(const application_command_create_t& _event)> _application_command_create);
	void on_resumed (std::function<void(const resumed_t& _event)> _resumed);
	void on_guild_role_create (std::function<void(const guild_role_create_t& _event)> _guild_role_create);
	void on_typing_start (std::function<void(const typing_start_t& _event)> _typing_start);
	void on_message_reaction_add (std::function<void(const message_reaction_add_t& _event)> _message_reaction_add);
	void on_guild_members_chunk (std::function<void(const guild_members_chunk_t& _event)> _guild_members_chunk);
	void on_message_reaction_remove (std::function<void(const message_reaction_remove_t& _event)> _message_reaction_remove);
	void on_guild_create (std::function<void(const guild_create_t& _event)> _guild_create);
	void on_channel_create (std::function<void(const channel_create_t& _event)> _channel_create);
	void on_message_reaction_remove_emoji (std::function<void(const message_reaction_remove_emoji_t& _event)> _message_reaction_remove_emoji);
	void on_message_delete_bulk (std::function<void(const message_delete_bulk_t& _event)> _message_delete_bulk);
	void on_guild_role_update (std::function<void(const guild_role_update_t& _event)> _guild_role_update);
	void on_guild_role_delete (std::function<void(const guild_role_delete_t& _event)> _guild_role_delete);
	void on_channel_pins_update (std::function<void(const channel_pins_update_t& _event)> _channel_pins_update);
	void on_message_reaction_remove_all (std::function<void(const message_reaction_remove_all_t& _event)> _message_reaction_remove_all);
	void on_voice_server_update (std::function<void(const voice_server_update_t& _event)> _voice_server_update);
	void on_guild_emojis_update (std::function<void(const guild_emojis_update_t& _event)> _guild_emojis_update);
	void on_presence_update (std::function<void(const presence_update_t& _event)> _presence_update);
	void on_webhooks_update (std::function<void(const webhooks_update_t& _event)> _webhooks_update);
	void on_guild_member_add (std::function<void(const guild_member_add_t& _event)> _guild_member_add);
	void on_invite_delete (std::function<void(const invite_delete_t& _event)> _invite_delete);
	void on_guild_update (std::function<void(const guild_update_t& _event)> _guild_update);
	void on_guild_integrations_update (std::function<void(const guild_integrations_update_t& _event)> _guild_integrations_update);
	void on_guild_member_update (std::function<void(const guild_member_update_t& _event)> _guild_member_update);
	void on_application_command_update (std::function<void(const application_command_update_t& _event)> _application_command_update);
	void on_invite_create (std::function<void(const invite_create_t& _event)> _invite_create);
	void on_message_update (std::function<void(const message_update_t& _event)> _message_update);
	void on_user_update (std::function<void(const user_update_t& _event)> _user_update);
	void on_message_create (std::function<void(const message_create_t& _event)> _message_create);
	void on_guild_ban_add (std::function<void(const guild_ban_add_t& _event)> _guild_ban_add);
	void on_guild_ban_remove (std::function<void(const guild_ban_remove_t& _event)> _guild_ban_remove);
	void on_integration_create (std::function<void(const integration_create_t& _event)> _integration_create);
	void on_integration_update (std::function<void(const integration_update_t& _event)> _integration_update);
	void on_integration_delete (std::function<void(const integration_delete_t& _event)> _integration_delete);

	/** Post a REST request. Where possible use a helper method instead like message_create */
	void post_rest(const std::string &endpoint, const std::string &parameters, http_method method, const std::string &postdata, json_encode_t callback, const std::string &filename = "", const std::string &filecontent = "");

	/** Get a message */
	void message_get(snowflake message_id, snowflake channel_id, command_completion_event_t callback);

	/** Get multiple messages */
	void messages_get(snowflake channel_id, snowflake around, snowflake before, snowflake after, snowflake limit, command_completion_event_t callback);

	/** Send a message to a channel. The callback function is called when the message has been sent */
	void message_create(const struct message &m, command_completion_event_t callback = {});

	/** Crosspost a message. The callback function is called when the message has been sent */
	void message_crosspost(snowflake message_id, snowflake channel_id, command_completion_event_t callback = {});

	/** Edit a message on a channel. The callback function is called when the message has been edited */
	void message_edit(const struct message &m, command_completion_event_t callback = {});

	/** Add a reaction to a message. The reaction string must be either an `emojiname:id` or a unicode character. */
	void message_add_reaction(const struct message &m, const std::string &reaction, command_completion_event_t callback = {});

	/** Delete own reaction from a message. The reaction string must be either an `emojiname:id` or a unicode character. */
	void message_delete_own_reaction(const struct message &m, const std::string &reaction, command_completion_event_t callback = {});

	/** Delete a user's reaction from a message. The reaction string must be either an `emojiname:id` or a unicode character */
	void message_delete_reaction(const struct message &m, snowflake user_id, const std::string &reaction, command_completion_event_t callback = {});

	/** Get reactions on a message for a particular emoji. The reaction string must be either an `emojiname:id` or a unicode character */
	void message_get_reactions(const struct message &m, const std::string &reaction, snowflake before, snowflake after, snowflake limit, command_completion_event_t callback);

	/** Delete all reactions on a message */
	void message_delete_all_reactions(const struct message &m, command_completion_event_t callback = {});

	/** Delete all reactions on a message using a particular emoji. The reaction string must be either an `emojiname:id` or a unicode character */
	void message_delete_reaction_emoji(const struct message &m, const std::string &reaction, command_completion_event_t callback = {});

	/** Delete a message from a channel. The callback function is called when the message has been edited */
	void message_delete(snowflake message_id, snowflake channel_id, command_completion_event_t callback = {});

	/** Bulk delete messages from a channel. The callback function is called when the message has been edited */
	void message_delete_bulk(const std::vector<snowflake> &message_ids, snowflake channel_id, command_completion_event_t callback = {});

	/** Get a channel */
	void channel_get(snowflake c, command_completion_event_t callback);

	/** Get all channels for a guild */
	void channels_get(snowflake guild_id, command_completion_event_t callback);

	/** Create a channel */
	void channel_create(const class channel &c, command_completion_event_t callback = {});

	/** Edit a channel. */
	void channel_edit(const class channel &c, command_completion_event_t callback = {});
	void channel_edit_position(const class channel &c, command_completion_event_t callback = {});

	/** Edit a channel permission */
	void channel_edit_permissions(const class channel &c, snowflake overwrite_id, uint32_t allow, uint32_t deny, bool member, command_completion_event_t callback = {});

	/** Delete a channel */
	void channel_delete(snowflake channel_id, command_completion_event_t callback = {});

	/** Get details about an invite */
	void invite_get(const std::string &invite, command_completion_event_t callback);

	/** Delete an invite */
	void invite_delete(const std::string &invite, command_completion_event_t callback = {});

	void channel_invites_get(const class channel &c, command_completion_event_t callback);

	/** Create invite for a channel */
	void channel_invite_create(const class channel &c, const class invite &i, command_completion_event_t callback = {});

	/** Get a channel's pins */
	void pins_get(snowflake channel_id, command_completion_event_t callback);

	/** Adds a recipient to a Group DM using their access token */
	void gdm_add(snowflake channel_id, snowflake user_id, const std::string &access_token, const std::string &nick, command_completion_event_t callback = {});

	/** Removes a recipient from a Group DM */
	void gdm_remove(snowflake channel_id, snowflake user_id, command_completion_event_t callback = {});

	/** Remove a permission from a channel */
	void channel_delete_permission(const class channel &c, snowflake overwrite_id, command_completion_event_t callback = {});

	/** Follow a news channel */
	void channel_follow_news(const class channel &c, snowflake target_channel_id, command_completion_event_t callback = {});

	/** Trigger channel typing indicator */
	void channel_typing(const class channel &c, command_completion_event_t callback = {});

	/** Pin a message */
	void message_pin(snowflake channel_id, snowflake message_id, command_completion_event_t callback = {});

	/** Unpin a message */
	void message_unpin(snowflake channel_id, snowflake message_id, command_completion_event_t callback = {});

	/** Get a guild */
	void guild_get(snowflake g, command_completion_event_t callback);

	/** Get a guild preview. Returns a guild object but only a subset of the fields will be populated. */
	void guild_get_preview(snowflake g, command_completion_event_t callback);

	/** Get a guild member */
	void guild_get_member(snowflake guild_id, snowflake user_id, command_completion_event_t callback);

	/** Get all guild members */
	void guild_get_members(snowflake guild_id, command_completion_event_t callback);

	/** Add guild member. Needs a specific oauth2 scope, from which you get the access_token. */
	void guild_add_member(const guild_member& gm, const std::string &access_token, command_completion_event_t callback = {});

	/** Edit the properties of an existing guild member. */
	void guild_edit_member(const guild_member& gm, command_completion_event_t callback = {});

	/** Change current user nickname */
	void guild_set_nickname(snowflake guild_id, const std::string &nickname, command_completion_event_t callback = {});

	/** Add role to guild member */
	void guild_member_add_role(snowflake guild_id, snowflake user_id, snowflake role_id, command_completion_event_t callback = {});

	/** Remove role from guild member */
	void guild_member_delete_role(snowflake guild_id, snowflake user_id, snowflake role_id, command_completion_event_t callback = {});

	/** Remove (kick) a guild member */
	void guild_member_delete(snowflake guild_id, snowflake user_id, command_completion_event_t callback = {});

	/** Add guild ban */
	void guild_ban_add(snowflake guild_id, snowflake user_id, uint32_t delete_message_days, const std::string &reason, command_completion_event_t callback = {});

	/** Delete guild ban */
	void guild_ban_delete(snowflake guild_id, snowflake user_id, command_completion_event_t callback = {});

	/** Get guild ban list */
	void guild_get_bans(snowflake guild_id, command_completion_event_t callback);

	/** Get single guild ban */
	void guild_get_ban(snowflake guild_id, snowflake user_id, command_completion_event_t callback);

	/** Get a template */
	void template_get(const std::string &code, command_completion_event_t callback);

	/** Create a new guild based on a template. */
	void guild_create_from_template(const std::string &code, const std::string &name, command_completion_event_t callback = {});

	/** Get guild templates */
	void guild_templates_get(snowflake guild_id, command_completion_event_t callback);

	/** Creates a template for the guild */
	void guild_template_create(snowflake guild_id, const std::string &name, const std::string &description, command_completion_event_t callback);

	/** Syncs the template to the guild's current state. */
	void guild_template_sync(snowflake guild_id, const std::string &code, command_completion_event_t callback = {});

	/** Modifies the template's metadata. */
	void guild_template_modify(snowflake guild_id, const std::string &code, const std::string &name, const std::string &description, command_completion_event_t callback = {});

	/** Deletes the template */
	void guild_template_delete(snowflake guild_id, const std::string &code, command_completion_event_t callback = {});

	/** Create a guild */
	void guild_create(const class guild &g, command_completion_event_t callback = {});

	/** Edit a guild */
	void guild_edit(const class guild &g, command_completion_event_t callback = {});

	/** Delete a guild */
	void guild_delete(snowflake guild_id, command_completion_event_t callback = {});

	/** Get all emojis for a guild */
	void guild_emojis_get(snowflake guild_id, command_completion_event_t callback);

	/** Get single guild emoji */
	void guild_emoji_get(snowflake guild_id, snowflake emoji_id, command_completion_event_t callback);

	/** Create single emoji. You must ensure that the emoji passed contained image data using the emoji::load_image() method. */
	void guild_emoji_create(snowflake guild_id, const class emoji& newemoji, command_completion_event_t callback = {});

	/** Edit a single emoji. You must ensure that the emoji passed contained image data using the emoji::load_image() method. */
	void guild_emoji_edit(snowflake guild_id, const class emoji& newemoji, command_completion_event_t callback = {});

	/** Delete a guild emoji */
	void guild_emoji_delete(snowflake guild_id, snowflake emoji_id, command_completion_event_t callback = {});

	/** Get prune counts */
	void guild_get_prune_counts(snowflake guild_id, const class prune& pruneinfo, command_completion_event_t callback);

	/** Begin guild prune */
	void guild_begin_prune(snowflake guild_id, const class prune& pruneinfo, command_completion_event_t callback = {});

	/** Get guild voice regions */
	void guild_get_voice_regions(snowflake guild_id, command_completion_event_t callback);

	/** Get guild invites */
	void get_guild_invites(snowflake guild_id, command_completion_event_t callback);

	/** Get guild itegrations */
	void guild_get_integrations(snowflake guild_id, command_completion_event_t callback);

	/** Modify guild integration */
	void guild_modify_integration(snowflake guild_id, const class integration &i, command_completion_event_t callback = {}); 

	/** Delete guild integration */
	void guild_delete_integration(snowflake guild_id, snowflake integration_id, command_completion_event_t callback = {});

	/** Sync guild integration */
	void guild_sync_integration(snowflake guild_id, snowflake integration_id, command_completion_event_t callback = {});

	/** Get guild widget */
	void guild_get_widget(snowflake guild_id, command_completion_event_t callback);

	/** Edit guild widget */
	void guild_edit_widget(snowflake guild_id, const class guild_widget &gw, command_completion_event_t callback = {});

	/** Get guild vanity url, if enabled */
	void guild_get_vanity(snowflake guild_id, command_completion_event_t callback);

	/** Create webhook */
	void create_webhook(const class webhook &w, command_completion_event_t callback = {});

	/** Get guild webhooks */
	void get_guild_webhooks(snowflake guild_id, command_completion_event_t callback);

	/** Get channel webhooks */
	void get_channel_webhooks(snowflake channel_id, command_completion_event_t callback);

	/** Get webhook */
	void get_webhook(snowflake webhook_id, command_completion_event_t callback);

	/** Get webhook using token */
	void get_webhook_with_token(snowflake webhook_id, const std::string &token, command_completion_event_t callback);

	/** Edit webhook */
	void edit_webhook(const class webhook& wh, command_completion_event_t callback = {});

	/** Edit webhook with token (token is encapsulated in the webhook object) */
	void edit_webhook_with_token(const class webhook& wh, command_completion_event_t callback = {});

	/** Delete webhook */
	void delete_webhook(snowflake webhook_id, command_completion_event_t callback = {});

	/** Delete webhook with token */
	void delete_webhook_with_token(snowflake webhook_id, const std::string &token, command_completion_event_t callback = {});

	/** Execute webhook */
	void execute_webhook(const class webhook &wh, const class message &m, command_completion_event_t callback = {});

	/** Edit webhook message */
	void edit_webhook_message(const class webhook &wh, const class message &m, command_completion_event_t callback = {});

	/** Delete webhook message */
	void delete_webhook_message(const class webhook &wh, snowflake message_id, command_completion_event_t callback = {});

	/** Get a role */
	void roles_get(snowflake guild_id, command_completion_event_t callback);

	/** Create a role */
	void role_create(const class role &r, command_completion_event_t callback = {});

	/** Edit a role */
	void role_edit(const class role &r, command_completion_event_t callback = {});
	void role_edit_position(const class role &r, command_completion_event_t callback = {});

	/** Delete a role */
	void role_delete(snowflake guild_id, snowflake role_id, command_completion_event_t callback = {});

	/** Get a user by id */
	void user_get(snowflake user_id, command_completion_event_t callback);

	/** Get current user */
	void current_user_get(command_completion_event_t callback);

	/** Get current user guilds */
	void current_user_get_guilds(command_completion_event_t callback);

	/** Edit current user */
	void current_user_edit(const std::string &nickname, const std::string& image_blob, image_type type, command_completion_event_t callback = {});

	/** Get current user DM channels */
	void current_user_get_dms(command_completion_event_t callback);

	/** Create DM channel */
	void create_dm_channel(snowflake user_id, command_completion_event_t callback = {});

	/** Leave a guild */
	void current_user_leave_guild(snowflake guild_id, command_completion_event_t callback = {});

	/** Get voice regions */
	void get_voice_regions(command_completion_event_t callback);

	/** Get gateway bot */
	void get_gateway_bot(command_completion_event_t callback);


};

};
