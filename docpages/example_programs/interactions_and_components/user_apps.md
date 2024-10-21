\page user-applications Creating User Apps

# What are User Apps?

A user app is a bot (application) which can be attached to a user's profile via oauth, rather than being invited to a guild via oauth. This is a relatively new feature
on Discord, and will allow you to have your bot to act like a utility for the user, so regardless of what guild they are in, or if they are in a DM with someone else,
they have access to your bot's commands and can issue them, potentially letting all users in that guild, or all users in the DM, see the replies from your slash commands.

\warning Do not confuse User Apps with **User Bots**. User Apps are a new and very well supported feature, whereas **User Bots** means connecting a user's token as a bot,
and is prohibited by the Discord TOS!

# Building the invite

To get started, you will need to configure your bot so that it will accept a user app invite. This is done via the [discord developer portal](https://discord.com/developers).

Click on your bot in the list of bots, and then choose **Installation** from the left hand menu. You must enable **User Install** if it is not already enabled.

Drop down the choices for **Install link** and change this to **Discord Provided Link**. The second box should auto-fill with an invite link. Note that this invite link
will likely only show the **client_id** value. For default install settings for **User Install**, choose the only possible option, **applications.commands** and for the
**Guild Install** section, choose the scopes **applications.commands** and **bot** as at least the bare minimum. You should also set the permissions your bot will use if it
is invited to a guild.

\note The permissions you pick in the Guild Install box only apply if your bot is invited to a guild, not for a user app!

If you have entered all the settings correctly the screen should look like the one below (except the **Administrator** permission - don't use this, enter actual permissions!):

\image html user_apps_1.png

# Inviting the application

You can now invite your bot to your profile. Follow the invite link at the top of the screen by clicking **copy** and pasting it into a web browser. You will be prompted
to either add the bot to your profile, or to a guild, as shown below. Choose to add the bot to your profile:

\image html user_apps_2.png

You may be prompted to prove you are not a robot (you aren't a robot, right? 🤖). Afterwards, the bot will be successfully added to your profile:

\image html user_apps_3.png

# Creating the program

From this point on, right now, your bot will do nothing as you haven't added any code yet to make it operate as a user app. This comes next. Below is an example bot
with one user application command.

There are several important things to note in this program:

* When adding a new slash command, you must use the dpp::slashcommand::set_interaction_contexts function to set where this command is visible. You can specify any
  one of three possible values to be added to the vector:
  * dpp::itc_guild: A standard slash command, visible in guild channels
  * dpp::itc_bot_dm: A standard slash command, accessible in DMs with the bot (this replaces the functionality of dpp::slashcommand::set_dm_permission)
  * dpp::itc_private_channel: A user application command, visible anywhere to the user who added it to their profile.
* When responding to the slash command, most things remain the same, except of course if you reply to a user app command most things relating to the guild
  will be uninitialised default values. This is because there is no guild to reference.
* You can use dpp::interaction::is_user_app_interaction() to determine if the interaction is initiated from a user app command, or a guild command.
  dpp::interaction::is_guild_interaction() does the inverse.
* Calling dpp::interaction::get_authorizing_integration_owner() with the parameter value dpp::ait_user_install will give you the dpp::snowflake ID of the
  user who has the app added to their profile, if a user is currently executing a user app command. If you call this function when not in a user app context,
  you will get an uninitialised snowflake value instead.

## Example Program

\include{cpp} user_apps.cpp

# Testing

If all goes to plan, your new command will be accessible everywhere!

\image html user_apps_4.png
