\page setting_status Setting the Bot's Status

A bot status is pretty cool, and it'd be cooler if you knew how to do it! This tutorial will cover how to set the bot status to say `Playing games!`, as well as covering how to set the status to the amount of guilds every two minutes.

\note dpp::get_guild_cache requires the bot to have the guild cache enabled, if your bot has this disabled then you can't use that. Instead, you should look to use dpp::cluster::current_application_get and get the `approximate_guild_count` from dpp::application in the callback.

First, we'll cover setting the bot status to `Playing games!`.

\include{cpp} setting_status1.cpp

If all went well, your bot should now be online and say this on members list!

\image html botonlinestatus.png

If you want to make your bot show as Do Not Disturb, then you could change dpp::ps_online to dpp::ps_dnd.
You can also play around with dpp::at_game, changing it to something like dpp::at_custom or dpp::at_listening!

Now, let's cover setting the bot status to say `Playing with x guilds!` every two minutes.

\note This example uses timers to update the status every 2 minutes. If you aren't familiar with D++'s own timers, please read \ref using_timers "this page on timers" before you continue.

\include{cpp} setting_status2.cpp

If you followed that well, your bot should now say this on members list!

\image html botonlinestatus2.png

If we then add our bot to another server and wait a bit, we'll see it updates like so:

\image html botonlinestatus3.png