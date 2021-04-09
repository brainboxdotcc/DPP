# REST Calls

* dpp::cluster::message_get (snowflake, snowflake, callback)


* dpp::cluster::messages_get (snowflake, snowflake around, snowflake before, snowflake after, snowflake limit, callback)


* dpp::cluster::message_create (message, callback)


* dpp::cluster::message_crosspost (snowflake, snowflake, callback)


* dpp::cluster::message_edit (message, callback)


* dpp::cluster::message_add_reaction (message,  callback)


* dpp::cluster::message_delete_own_reaction (message,  callback)


* dpp::cluster::message_delete_reaction (message, snowflake, callback)


* dpp::cluster::message_get_reactions (message, snowflake before, snowflake after, snowflake, callback)


* dpp::cluster::message_delete_all_reactions (message, callback)


* dpp::cluster::message_delete_reaction_emoji (message,  callback)


* dpp::cluster::message_delete (snowflake, snowflake, callback)


* dpp::cluster::message_delete_bulk (vector<snowflake>, snowflake, callback)


* dpp::cluster::channel_get (snowflake c, callback)


* dpp::cluster::channels_get (snowflake, callback)


* dpp::cluster::channel_create (channel, callback)


* dpp::cluster::channel_edit (channel, callback)


* dpp::cluster::channel_edit_position (channel, callback)


* dpp::cluster::channel_edit_permissions (channel, snowflake, uint32_t, uint32_t, bool member, callback)


* dpp::cluster::channel_delete (snowflake, callback)


* dpp::cluster::invite_get (, callback)


* dpp::cluster::invite_delete (, callback)


* dpp::cluster::channel_invites_get (channel, callback)


* dpp::cluster::channel_invite_create (channel, invite, callback)


* dpp::cluster::pins_get (snowflake, callback)


* dpp::cluster::gdm_add (snowflake, snowflake,  callback)


* dpp::cluster::gdm_remove (snowflake, snowflake, callback)


* dpp::cluster::channel_delete_permission (channel, snowflake, callback)


* dpp::cluster::channel_follow_news (channel, snowflake, callback)


* dpp::cluster::channel_typing (channel, callback)


* dpp::cluster::message_pin (snowflake, snowflake, callback)


* dpp::cluster::message_unpin (snowflake, snowflake, callback)


* dpp::cluster::guild_get (snowflake, callback)


* dpp::cluster::guild_get_preview (snowflake, callback)


* dpp::cluster::guild_get_member (snowflake, snowflake, callback)


* dpp::cluster::guild_get_members (snowflake, callback)


* dpp::cluster::guild_add_member (guild_member,  callback)


* dpp::cluster::guild_edit_member (guild_member, callback)


* dpp::cluster::guild_set_nickname (snowflake, callback)


* dpp::cluster::guild_member_add_role (snowflake, snowflake, snowflake, callback)


* dpp::cluster::guild_member_delete_role (snowflake, snowflake, snowflake, callback)


* dpp::cluster::guild_member_delete (snowflake, snowflake, callback)


* dpp::cluster::guild_ban_add (snowflake, snowflake, uint32_t,  callback)


* dpp::cluster::guild_ban_delete (snowflake, snowflake, callback)


* dpp::cluster::guild_get_bans (snowflake, callback)


* dpp::cluster::guild_get_ban (snowflake, snowflake, callback)


* dpp::cluster::template_get (, callback)


* dpp::cluster::guild_create_from_template (,  callback)


* dpp::cluster::guild_templates_get (snowflake, callback)


* dpp::cluster::guild_template_create (snowflake,  callback)


* dpp::cluster::guild_template_sync (snowflake, callback)


* dpp::cluster::guild_template_modify (snowflake,, callback)


* dpp::cluster::guild_template_delete (snowflake, callback)


* dpp::cluster::guild_create (guild, callback)


* dpp::cluster::guild_edit (guild, callback)


* dpp::cluster::guild_delete (snowflake, callback)


* dpp::cluster::guild_emojis_get (snowflake, callback)


* dpp::cluster::guild_emoji_get (snowflake, snowflake, callback)


* dpp::cluster::guild_emoji_create (snowflake, emoji, callback)


* dpp::cluster::guild_emoji_edit (snowflake, emoji, callback)


* dpp::cluster::guild_emoji_delete (snowflake, snowflake, callback)


* dpp::cluster::guild_get_prune_counts (snowflake, prune, callback)


* dpp::cluster::guild_begin_prune (snowflake, prune, callback)


* dpp::cluster::guild_get_voice_regions (snowflake, callback)


* dpp::cluster::get_guild_invites (snowflake, callback)


* dpp::cluster::guild_get_integrations (snowflake, callback)


* dpp::cluster::guild_modify_integration (snowflake, integration, callback)


* dpp::cluster::guild_delete_integration (snowflake, snowflake, callback)


* dpp::cluster::guild_sync_integration (snowflake, snowflake, callback)


* dpp::cluster::guild_get_widget (snowflake, callback)


* dpp::cluster::guild_edit_widget (snowflake, guild_widget, callback)


* dpp::cluster::guild_get_vanity (snowflake, callback)


* dpp::cluster::create_webhook (webhook, callback)


* dpp::cluster::get_guild_webhooks (snowflake, callback)


* dpp::cluster::get_channel_webhooks (snowflake, callback)


* dpp::cluster::get_webhook (snowflake, callback)


* dpp::cluster::get_webhook_with_token (snowflake, callback)


* dpp::cluster::edit_webhook (webhook, callback)


* dpp::cluster::edit_webhook_with_token (webhook, callback)


* dpp::cluster::delete_webhook (snowflake, callback)


* dpp::cluster::delete_webhook_with_token (snowflake, callback)


* dpp::cluster::execute_webhook (webhook, message, callback)


* dpp::cluster::edit_webhook_message (webhook, message, callback)


* dpp::cluster::delete_webhook_message (webhook, snowflake, callback)


* dpp::cluster::roles_get (snowflake, callback)


* dpp::cluster::role_create (role, callback)


* dpp::cluster::role_edit (role, callback)


* dpp::cluster::role_edit_position (role, callback)


* dpp::cluster::role_delete (snowflake, snowflake, callback)


* dpp::cluster::user_get (snowflake, callback)


* dpp::cluster::current_user_get (callback)


* dpp::cluster::current_user_get_guilds (callback)


* dpp::cluster::current_user_edit (, , image_type type, callback)


* dpp::cluster::current_user_get_dms (callback)


* dpp::cluster::create_dm_channel (snowflake, callback)


* dpp::cluster::current_user_leave_guild (snowflake, callback)


* dpp::cluster::get_voice_regions (callback)


* dpp::cluster::get_gateway_bot (callback)


