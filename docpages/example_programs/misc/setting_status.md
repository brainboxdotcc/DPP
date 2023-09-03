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

\note This example makes use of callbacks. For more information about that, visit \ref callback-functions "Using Callback Functions".

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main()
{
    /* Create the bot */
    dpp::cluster bot("token");

    bot.on_log(dpp::utility::cout_logger());

    bot.on_ready([&bot](const dpp::ready_t& event) {
        /* We don't need the run_once here as we're not registering commands! */

        std::thread status_thread([&bot]() {
            /* We want to infinitely loop here, so we always update the status */
            while (true) {
                /* Tell the thread to sleep for 120 seconds, this is so we don't ask for the guilds too often meaning we avoid being rate-limited */
                std::this_thread::sleep_for(std::chrono::seconds(120));

                /* Get the guilds that the bot is in */
                bot.current_user_get_guilds([&bot](const dpp::confirmation_callback_t& callback) {
                    if(callback.is_error()) {
                        return;
                    }

                    /* Update the presence */
                    bot.set_presence(dpp::presence(dpp::presence_status::ps_online, dpp::activity_type::at_game, "with " + std::to_string(std::get<dpp::guild_map>(callback.value).size()) + " guilds!"));
                });
            }
        });

        /* This makes the thread completely separate, so it's not reliant on the main thread (meaning you can still do anything else you want, whilst this happens!) */
        status_thread.detach();
    });

    bot.start(dpp::st_wait);

    return 0;
}
~~~~~~~~~~

If you followed that well, your bot should now say this on members list!

\image html botonlinestatus2.png

If we then add our bot to another server and wait a bit, we'll see it updates like so:

\image html botonlinestatus3.png