\page checking-member-permissions Checking Permissions

Of course most people typically iterate over the roles of a member to check for a permission. But there is a helper method for this: dpp::guild::base_permissions retrieves a member's permissions, taking into account role permissions **and** the server owner.

For total member permissions including channel overwrites use either the dpp::channel::get_user_permissions or dpp::guild::permission_overwrites method. Both do the same under the hood.

They all return a dpp::permission class, which is a wrapper around a permission bitmask with several helpful methods for easier manipulation and checking of permissions. This bitmask contains flags from the dpp::permissions enum.

Demonstration:

```cpp
dpp::channel* c = dpp::find_channel(some_channel_id);
if (c && c->get_user_permissions(member).can(dpp::p_send_messages)) {
	//...
}
```

### Role Hierarchy

The recommended and correct way to compare for roles in the hierarchy is using the comparison operators (`<`, `>`) on the dpp::role objects themselves. Keep in mind that multiple roles can have the same position number. As a result, comparing roles by position alone can lead to subtle bugs when checking for role hierarchy.

For example let's say you have a ban command, and want to make sure that any issuer of the command can only ban members of lower position than their own highest role:

```cpp
bot.on_interaction_create([](const dpp::interaction_create_t& event) {
    dpp::snowflake target_id = std::get<dpp::snowflake>(event.get_parameter("user"));
    dpp::guild_member target = event.command.get_resolved_member(target_id);

    for (dpp::snowflake issuer_role_id : event.command.member.get_roles()) {
        auto issuer_role = dpp::find_role(issuer_role_id);
        if (issuer_role == nullptr) continue;
        for (dpp::snowflake target_role_id : target.get_roles()) {
            auto target_role = dpp::find_role(target_role_id);
            if (target_role == nullptr) continue;
            if (target_role > issuer_role) {
                event.reply("You can't ban someone whose role is higher than yours!");
                return;
            }
        }
    }
});
```

## Permissions in Interaction Events

### Default Command Permissions

Discord's intended way of managing permissions for commands is through "default member permissions". In a nutshell you tell Discord which permissions a user must have to use the command. Discord completely hides the command for members who don't have the required permissions. You set them using dpp::slashcommand::set_default_permissions when creating or updating a command.

The corresponding code to create a command with default permissions would look something like this:

```cpp
dpp::slashcommand command("ban", "Ban a member", bot.me.id);

command.set_default_permissions(dpp::p_ban_members); // set permissions that are required by default here

command.add_option(dpp::command_option(dpp::co_user, "user", "The user to ban", true));
command.add_option(dpp::command_option(dpp::co_string, "reason", "The reason for banning", true));

bot.global_command_create(command);
```

You can set the default member permissions to "0" to disable the command for everyone except admins by default.

For more customization for server owners, they can override these permissions by their own restrictions in the server settings. This is why they are referred to as "default" permissions.

### Checking Permissions on Your Own

When using default permissions you don't necessarily need to check the issuing user for any permissions in the interaction event as Discord handles all that for you. However, if you don't want server admins to be able to override the command restrictions, you can make those permission checks on your own.

To check if a member has certain permissions during interaction events, the easiest way is to use the dpp::interaction::get_resolved_permission function. The resolved list contains associated structures for the command and does not rely on the cache or require any extra API calls. Additionally, the permissions in the resolved set are pre-calculated by Discord and taking into account channel overwrites, roles and admin privileges. So, there's no need to loop through roles or stuff like that.

Let's imagine the following scenario:

You have a ban command and want to make sure the issuer has the ban permission.

```cpp
bot.on_interaction_create([](const dpp::interaction_create_t& event) {
	dpp::permission perms = event.command.get_resolved_permission(event.command.usr.id);
	if (!perms.can(dpp::p_ban_members)) {
		event.reply("You don't have the required permissions to ban someone!");
		return;
	}
});
```

### From Parameters

The resolved set also contains the permissions of members from command parameters.

For example, let's say you want to prohibit people from banning server admins with your ban command.

Get the user ID from the parameters and pass it to the `get_resolved_permission` method:

```cpp
bot.on_interaction_create([](const dpp::interaction_create_t& event) {
	dpp::snowflake user_id = std::get<dpp::snowflake>(event.get_parameter("user"));
	dpp::permission perms = event.command.get_resolved_permission(user_id);
	if (perms.has(dpp::p_administrator)) {
		event.reply("You can't ban Admins!");
		return;
	}
});
```

### The Bot's Permissions

You also might want to check if the bot itself has the ban permission before processing the command further. You can access the bot's permissions in the dpp::interaction::app_permissions field.

```cpp
bot.on_interaction_create([](const dpp::interaction_create_t& event) {
	if (!event.command.app_permissions.can(dpp::p_ban_members)) {
		event.reply("The bot doesn't have the required permission to ban anyone!");
		return;
	}
});
```

### Things to Keep in Mind

When replying to interactions using dpp::interaction_create_t::reply, you do **not** need to manually check whether the bot has permission to send messages. A bot always has permissions to reply to an interaction.
