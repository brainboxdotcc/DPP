# REST Calls

Each REST call has a different effect on discord. Each is considered a distinct "command" with a reply. All REST calls take time to complete. Depending on current load, discord latency, and rate limits, they may return almost instantly to the callback function, or may take seconds to complete.

Click each item to find details on the specifics of the call.

### dpp::cluster::message_get (snowflake, snowflake, callback)

Get a single message


### dpp::cluster::messages_get (snowflake, snowflake around, snowflake before, snowflake after, snowflake limit, callback)

Get multiple messages


### dpp::cluster::message_create (message, callback)

Send a message (may include embeds, files)


### dpp::cluster::message_crosspost (snowflake, snowflake, callback)

Crosspost a news message to other servers


### dpp::cluster::message_edit (message, callback)

Update a mesage's content (including embeds)


### dpp::cluster::message_add_reaction (message,  callback)

Add a reaction to a message


### dpp::cluster::message_delete_own_reaction (message,  callback)

Delete bot's reaction from a message

### dpp::cluster::message_delete_reaction (message, snowflake, callback)

Delete another user's reaction from a message


### dpp::cluster::message_get_reactions (message, snowflake before, snowflake after, snowflake, callback)

Get all reactions for a message

### dpp::cluster::message_delete_all_reactions (message, callback)

Detel all reactions for a message

### dpp::cluster::message_delete_reaction_emoji (message,  callback)

Delete all reactions for a message that reacted with a particular emoji

### dpp::cluster::message_delete (snowflake, snowflake, callback)

Delete a message


### dpp::cluster::message_delete_bulk (vector<snowflake>, snowflake, callback)

Delete a set of messages

### dpp::cluster::channel_get (snowflake c, callback)

Get a channel


### dpp::cluster::channels_get (snowflake, callback)

Get all channels for a guild

### dpp::cluster::channel_create (channel, callback)

Create a channel

### dpp::cluster::channel_edit (channel, callback)

Edit a channel

### dpp::cluster::channel_edit_position (channel, callback)

Edit a channel's position in the channel list

### dpp::cluster::channel_edit_permissions (channel, snowflake, uint32_t, uint32_t, bool member, callback)

Edit permissions for a channel

### dpp::cluster::channel_delete (snowflake, callback)

Delete a channel

### dpp::cluster::invite_get (code, callback)

Get an invite for a channel

### dpp::cluster::invite_delete (code, callback)

Delete an invite from a channel

### dpp::cluster::channel_invites_get (channel, callback)

Get all invites for a channel

### dpp::cluster::channel_invite_create (channel, invite, callback)

Create an invite for a channel

### dpp::cluster::pins_get (snowflake, callback)

Get pinned messages for a channel


### dpp::cluster::gdm_add (snowflake, snowflake,  callback)

Add user to a group DM


### dpp::cluster::gdm_remove (snowflake, snowflake, callback)

Remove user from a group DM

### dpp::cluster::channel_delete_permission (channel, snowflake, callback)

Delete permission entry from a channel

### dpp::cluster::channel_follow_news (channel, snowflake, callback)

Follow a news channel

### dpp::cluster::channel_typing (channel, callback)

Indicate to a channel that the bot is typing

### dpp::cluster::message_pin (snowflake, snowflake, callback)

Pin a message

### dpp::cluster::message_unpin (snowflake, snowflake, callback)

Unpin a message

### dpp::cluster::guild_get (snowflake, callback)

Get a guild

### dpp::cluster::guild_get_preview (snowflake, callback)

Get a guild's preview

### dpp::cluster::guild_get_member (snowflake, snowflake, callback)

Get a guild member

### dpp::cluster::guild_get_members (snowflake, callback)

Get all guild members

### dpp::cluster::guild_add_member (guild_member,  callback)

Add a guild member (requires OAUTH2)

### dpp::cluster::guild_edit_member (guild_member, callback)

Edit a guild member

### dpp::cluster::guild_set_nickname (snowflake, callback)

Set a guild member's nickname

### dpp::cluster::guild_member_add_role (snowflake, snowflake, snowflake, callback)

Add a role to a member of a guild

### dpp::cluster::guild_member_delete_role (snowflake, snowflake, snowflake, callback)

Delete a role from a member of a guild

### dpp::cluster::guild_member_delete (snowflake, snowflake, callback)

Delete (kick!) a guild member

### dpp::cluster::guild_ban_add (snowflake, snowflake, uint32_t,  callback)

Add a ban for a user

### dpp::cluster::guild_ban_delete (snowflake, snowflake, callback)

Delete a ban

### dpp::cluster::guild_get_bans (snowflake, callback)

Get the ban list

### dpp::cluster::guild_get_ban (snowflake, snowflake, callback)

Get one ban

### dpp::cluster::template_get (code, callback)

Get a guild template

### dpp::cluster::guild_create_from_template (code,  callback)

Create a guild from a template

### dpp::cluster::guild_templates_get (snowflake, callback)

Get all guild templates in a guild

### dpp::cluster::guild_template_create (snowflake,  callback)

Create a guild template from a guild

### dpp::cluster::guild_template_sync (snowflake, callback)

Synchronise a guild template with current settings

### dpp::cluster::guild_template_modify (snowflake,, callback)

Modify an existing guild template

### dpp::cluster::guild_template_delete (snowflake, callback)

Delete a guild template

### dpp::cluster::guild_create (guild, callback)

Create a guild (only applicable for bots on less than 10 guilds)

### dpp::cluster::guild_edit (guild, callback)

Edit a guild

### dpp::cluster::guild_delete (snowflake, callback)

Delete a guild

### dpp::cluster::guild_emojis_get (snowflake, callback)

Get all guild emojis

### dpp::cluster::guild_emoji_get (snowflake, snowflake, callback)

Get an individual guild emoji

### dpp::cluster::guild_emoji_create (snowflake, emoji, callback)

Create a guild emoji

### dpp::cluster::guild_emoji_edit (snowflake, emoji, callback)

Edit a guild emoji

### dpp::cluster::guild_emoji_delete (snowflake, snowflake, callback)

Delete a guild emoji

### dpp::cluster::guild_get_prune_counts (snowflake, prune, callback)

Get the calculated counts for a prune (mass kick) operation

### dpp::cluster::guild_begin_prune (snowflake, prune, callback)

Begin a guild prune (mass kick) operation

### dpp::cluster::guild_get_voice_regions hsnowflake, callback)

Get a list of voice regions applicable to a guild

### dpp::cluster::get_guild_invites (snowflake, callback)

Get a list of invites for a guild

### dpp::cluster::guild_get_integrations (snowflake, callback)

Get a list of integrations on a guild

### dpp::cluster::guild_modify_integration (snowflake, integration, callback)

Modify a guild integration

### dpp::cluster::guild_delete_integration (snowflake, snowflake, callback)

Delete a guild integration

### dpp::cluster::guild_sync_integration (snowflake, snowflake, callback)

Sync a guild integration with the guild

### dpp::cluster::guild_get_widget (snowflake, callback)

Get the guilds widget settings

### dpp::cluster::guild_edit_widget (snowflake, guild_widget, callback)

Modify the guilds widget settings

### dpp::cluster::guild_get_vanity (snowflake, callback)

Get a guild's vanity URL if applicable

### dpp::cluster::create_webhook (webhook, callback)

Create a webhook

### dpp::cluster::get_guild_webhooks (snowflake, callback)

Get a list of all webhooks

### dpp::cluster::get_channel_webhooks (snowflake, callback)

Get webhooks for a channel

### dpp::cluster::get_webhook (snowflake, callback)

Get a specific webhook

### dpp::cluster::get_webhook_with_token (snowflake, callback)

Get a webhook using its access token

### dpp::cluster::edit_webhook (webhook, callback)

Edit a webhook

### dpp::cluster::edit_webhook_with_token (webhook, callback)

Edit a webhook using its access token

### dpp::cluster::delete_webhook (snowflake, callback)

Delete a webhook

### dpp::cluster::delete_webhook_with_token (snowflake, callback)

Delete a webhook using its access token

### dpp::cluster::execute_webhook (webhook, message, callback)

Execute a webhook, causing it to send a message

### dpp::cluster::edit_webhook_message (webhook, message, callback)

Edit a message previously sent by a webhook

### dpp::cluster::delete_webhook_message (webhook, snowflake, callback)

Delete a message previously sent by a webhook

### dpp::cluster::roles_get (snowflake, callback)

Get all roles for a guild

### dpp::cluster::role_create (role, callback)

Create a role on a guild

### dpp::cluster::role_edit (role, callback)

Edit a role on a guild

### dpp::cluster::role_edit_position (role, callback)

Edit a role's position in the list of roles

### dpp::cluster::role_delete (snowflake, snowflake, callback)

Delete a role

### dpp::cluster::user_get (snowflake, callback)

Get information on a user

### dpp::cluster::current_user_get (callback)

Get the information on the current (bot) user

### dpp::cluster::current_user_get_guilds (callback)

Get the information on the current (bot) user's guilds. Does not work if the bot is on over 100,000 guilds.

### dpp::cluster::current_user_edit (settings, image_data, image_type, callback)

Change the settings of the current (bot) user

### dpp::cluster::current_user_get_dms (callback)

Get the current (bot) user's dm channels

### dpp::cluster::create_dm_channel (snowflake, callback)

Create a DM channel

### dpp::cluster::current_user_leave_guild (snowflake, callback)

Leave a guild

### dpp::cluster::get_voice_regions (callback)

Get a global list of voice regions applicable to all users

### dpp::cluster::get_gateway_bot (callback)

Get the information required for a bot to register with the websocket gateway

