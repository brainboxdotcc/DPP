\page setting_status Setting the bot's status

A bot status is pretty cool, and it'd be cooler if you knew how to do it! This tutorial will cover how to set the bot status to say `Playing games!`.

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