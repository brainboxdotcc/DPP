\page context-menu Context Menus

Context menus are application commands that appear on the context menu (right click or tap) of users or messages to perform context-specific actions. They can be created using dpp::slashcommand. Once you create a context menu, try right-clicking either a user or message to see it in your server!

\image html context_menu_user_command.png

\note This example sets the command as the type dpp::ctxm_user which can only be used by right clicking on a user. To make it appear on a message, you'll want to switch the type to dpp::ctxm_message and listen for the `on_message_context_menu` (dpp::message_context_menu_t) event.

The following example shows how to create and handle **user context menus** for message context menus, read the notice above.

\include{cpp} context_menus.cpp

It registers a guild command that can be called by right-clicking a user and clicking on the created menu.

\image html context_menu_user_command_showcase.png