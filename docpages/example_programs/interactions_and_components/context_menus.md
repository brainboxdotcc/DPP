\page context-menu Context Menus

Context menus are application commands that appear on the context menu (right click or tap) of users or messages to perform context-specific actions. They can be created using `dpp::slashcommand`. Once you create a context menu, try right-clicking either a user or message to see it in your server!

\image html context_menu_user_command.png

The following example shows how to create and handle **user context menus**.

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <iostream>

int main()
{
    dpp::cluster bot("token");

    bot.on_log(dpp::utility::cout_logger());

    bot.on_ready([&bot](const dpp::ready_t &event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            /* Register the command */
            bot.guild_command_create(
		dpp::slashcommand()
					.set_type(dpp::ctxm_user)
                    .set_name("High Five")
                    .set_application_id(bot.me.id),
                857692897221033129 // you need to put your guild-id in here
            );
        }
    });

    /* Use the on_user_context_menu event to look for user context menu actions */
    bot.on_user_context_menu([&](const dpp::user_context_menu_t &event) {
         /* check if the context menu name is High Five */
         if (event.command.get_command_name() == "High Five") {
             dpp::user user = event.get_user(); // the user who the command has been issued on
             dpp::user author = event.command.usr; // the user who clicked on the context menu
             event.reply(author.get_mention() + " slapped " + user.get_mention());
         }
    });

    /* Start bot */
    bot.start(false);

    return 0;
}
~~~~~~~~~~

It registers a guild command that can be called by right-click a user and click on the created menu.

\image html context_menu_user_command_showcase.png
