# Event Handlers

Event handlers can be attached to from your program to be notified of updates to discord. Each one can be hooked by attaching a lambda, std::function or std::bind or equivalent to the event with one of the functions below, for example to be notified of messages being sent, assuming you have an instance of dpp::cluster called `bot`:

~~~~~~~~~~~~~{.cpp}
bot.on_message_create([&bot](const dpp::message_create_t & event) {
	bot.log(dpp::ll_info, fmt::format("Message Received: {}", event.msg->content));
});
~~~~~~~~~~~~~

### dpp::cluster::on_voice_state_update ()

Be notified of a change in voice state for a user

### dpp::cluster::on_log ()

Be notified of log lines to implement your own logging of bot activity

### dpp::cluster::on_interaction_create ()

Be notified of use of a slash command on a server

### dpp::cluster::on_guild_delete ()

Be notified of a guild being deleted (bot kicked, left, or server unavailable)

### dpp::cluster::on_channel_delete ()

Be notified of a channel being deleted

### dpp::cluster::on_channel_update ()

Be notified of a channel being edited

### dpp::cluster::on_ready ()

Be notified of a shard becoming ready

### dpp::cluster::on_message_delete ()

Be notified of a message being deleted

### dpp::cluster::on_application_command_delete ()

Be notified of an application command (slash command) being deleted

### dpp::cluster::on_guild_member_remove ()

Be notified of a guild member being removed (kicked or leaving)

### dpp::cluster::on_application_command_create ()

Be notified of an application command (slash command) being created

### dpp::cluster::on_resumed ()

Be notified of a connection being resumed

### dpp::cluster::on_guild_role_create ()

Be notified of a guild role being created

### dpp::cluster::on_typing_start ()

Be notified of a user typing

### dpp::cluster::on_message_reaction_add ()

Be notified of a reaction on a message

### dpp::cluster::on_guild_members_chunk ()

Be notified of a set of guild member information being received

### dpp::cluster::on_message_reaction_remove ()

Be notified of a reaction being removed from a message

### dpp::cluster::on_guild_create ()

Be notified of a guild being created/joined

### dpp::cluster::on_channel_create ()

Be notified of a channel being created

### dpp::cluster::on_message_reaction_remove_emoji ()

Be notified of all message reactions being removed on a message for one particular emoji

### dpp::cluster::on_message_delete_bulk ()

Be notified of a set of messages being deleted

### dpp::cluster::on_guild_role_update ()

Be notified of a role being edited

### dpp::cluster::on_guild_role_delete ()

Be notified of a role being deleted

### dpp::cluster::on_channel_pins_update ()

Be notified of the channel pins for a channel being updated

### dpp::cluster::on_message_reaction_remove_all ()

Be notified of a message reaction being removed

### dpp::cluster::on_voice_server_update ()

Be notified of the voice server endpoint being updated

### dpp::cluster::on_guild_emojis_update ()

Be notified of a guild's emojis being updated

### dpp::cluster::on_presence_update ()

Be notified of a user's presence/activity being updated

### dpp::cluster::on_webhooks_update ()

Be notified of a webhook being updated


### dpp::cluster::on_guild_member_add ()

Be notified of a guild member being added

### dpp::cluster::on_invite_delete ()

Be notified of an invite being deleted

### dpp::cluster::on_guild_update ()

Be notified of a guild being updated

### dpp::cluster::on_guild_integrations_update ()

Be notified of a guild integration being updated

### dpp::cluster::on_guild_member_update ()

Be notified of a guild member being updated

### dpp::cluster::on_application_command_update ()

Be notified of an application command (slash command) being updated

### dpp::cluster::on_invite_create ()

Be notified of an invite being created

### dpp::cluster::on_message_update ()

Be notified of a message being edited

### dpp::cluster::on_user_update ()

Be notified of a user's information being updated

### dpp::cluster::on_message_create ()

Be notified of a message being sent

### dpp::cluster::on_guild_ban_add ()

Be notified of a ban being added

### dpp::cluster::on_guild_ban_remove ()

Be notified of a ban being removed

### dpp::cluster::on_integration_create ()

Be notified of an integration being created

### dpp::cluster::on_integration_update ()

Be notified of an integration being updated

### dpp::cluster::on_integration_delete ()

Be notified of an integration being deleted 

