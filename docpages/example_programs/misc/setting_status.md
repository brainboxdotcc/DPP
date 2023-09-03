\page setting_status Setting the bot's status

A bot status is pretty cool, and it'd be cooler if you knew how to do it! This tutorial will cover how to set the bot status to say `Playing games!`, as well as covering how to set the status to the amount of guilds every two minutes.

First, we'll cover setting the bot status to `Playing games!`.

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main()
{
    /* Create the bot */
    dpp::cluster bot("token");

    bot.on_log(dpp::utility::cout_logger());

    bot.on_ready([&bot](const dpp::ready_t& event) {
        /* We don't need the run_once here as we're not registering commands! */

        /* Set the bot presence as online and "Playing..."! */
        bot.set_presence(dpp::presence(dpp::presence_status::ps_online, dpp::activity_type::at_game, "games!"));
    });

    bot.start(dpp::st_wait);

    return 0;
}
~~~~~~~~~~

If all went well, your bot should now be online and say this on members list!

\image html botonlinestatus.png

Now, let's cover setting the bot status to say `Playing with x guilds!` every two minutes.

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main()
{
    /* Create the bot */
    dpp::cluster bot("token");

    bot.on_log(dpp::utility::cout_logger());

    bot.on_ready([&bot](const dpp::ready_t& event) {
        /* We put our status updating inside "run_once" so that multiple shards don't try do this as "set_presence" updates all the shards. */
        if (dpp::run_once<struct register_bot_commands>()) {
            /* We update the presence now as the timer will do the first execution after the x amount of seconds we specify */
            bot.set_presence(dpp::presence(dpp::presence_status::ps_online, dpp::activity_type::at_game, "with " + std::to_string(dpp::get_guild_cache()->count()) + " guilds!"));

            /* Create a timer that runs every 120 seconds, that sets the status */
            bot.start_timer([&bot](const dpp::timer& timer) {
                bot.set_presence(dpp::presence(dpp::presence_status::ps_online, dpp::activity_type::at_game, "with " + std::to_string(dpp::get_guild_cache()->count()) + " guilds!"));
            }, 120);
        }
    });

    bot.start(dpp::st_wait);

    return 0;
}
~~~~~~~~~~

If you followed that well, your bot should now say this on members list!

\image html botonlinestatus2.png

If we then add our bot to another server and wait a bit, we'll see it updates like so:

\image html botonlinestatus3.png