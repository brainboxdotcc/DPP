\page commandhandler Using a Command Handler Object

If you have many commands in your bot, and want to handle commands from multiple sources, you should consider instantiating a dpp::commandhandler object. This object can be used to automatically route
commands and their parameters to functions in your program. A simple example of using this object to route commands is shown below, and will
route both the /ping (global slash command) and .ping (prefixed channel message command) to a lambda where a reply can be generated.

\note This example automatically hooks the dpp::cluster::on_message_create and dpp::cluster::on_slashcommand events. This can be overridden if needed to allow you to still make use of these functions for your own code, if you need to do this please see the constructor documentation for dpp::commandhandler.

Note that because the dpp::commandhandler::add_command method accepts a `std::function` as the command handler, you may point a command handler
at a simple lambda (as shown in this example), a function pointer, or an instantiated class method of an object. This is extremely flexible
and allows you to decide how and where commands should be routed, either to an object oriented system or to a lambda based system.

\warning As of [August 30th, 2022](https://support-dev.discord.com/hc/en-us/articles/6025578854295-Why-We-Moved-to-Slash-Commands), you are advised to only be using slash commands, not messages for commands. To prevent the command handler from handling commands with messages, you should only use the "/" prefix. If you wish to still use messages for commands, this tutorial will still cover it but, again, it is discouraged by Discord.

\include{cpp} commandhandler.cpp
