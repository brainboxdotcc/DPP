\page context-menu Context Menus

Context menus are application commands that appear on the context menu (right click or tap) of users or messages to perform context-specific actions. They can be created using dpp::slashcommand. Once you create a context menu, try right-clicking either a user or message to see it in your server!

\image html context_menu_user_command.png

\note This example sets the command as the type dpp::ctxm_user which can only be used by right clicking on a user. To make it appear on a message, you'll want to switch the type to dpp::ctxm_message and listen for the `on_message_context_menu` (dpp::message_context_menu_t) event.

The following example shows how to create and handle **user context menus** for message context menus, read the notice above.

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <iostream>

int main()
{
    dpp::cluster bot("token");

    bot.on_log(dpp::utility::cout_logger());

    /* Use the on_user_context_menu event to look for user context menu actions */
    bot.on_user_context_menu([&](const dpp::user_context_menu_t& event) {

         /* check if the context menu name is High Five */
         if (event.command.get_command_name() == "high five") {
             dpp::user user = event.get_user(); // the user who the command has been issued on
             dpp::user author = event.command.get_issuing_user(); // the user who clicked on the context menu
             event.reply(author.get_mention() + " slapped " + user.get_mention());
         }
    });

    bot.on_ready([&bot](const dpp::ready_t &event) {
        if (dpp::run_once<struct register_bot_commands>()) {

            /* Create the command */
            dpp::slashcommand command;
            command.set_name("High Five");
            command.set_application_id(bot.me.id); 
            command.set_type(dpp::ctxm_user);

            /* Register the command */
		    bot.guild_command_create(command, 857692897221033129); /* Replace this with the guild id you want */
        }
    });

    /* Start bot */
    bot.start(dpp::st_wait);

    return 0;
}
~~~~~~~~~~

It registers a guild command that can be called by right-clicking a user and clicking on the created menu.

\image html context_menu_user_command_showcase.png