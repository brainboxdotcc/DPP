\page clearing_slashcommands Clearing Registered Commands

After a while of creating commands, you may start to wonder "hm, how can I clear these?". Well, this tutorial covers it! All you have to do is simply call dpp::cluster::global_bulk_command_delete or dpp::cluster::guild_bulk_command_delete.

\note Clearing **global commands** will only clear commands that were **made globally**, same goes for the opposite way round. The example will demonstrate using both functions.

\include{cpp} clearing_slashcommands.cpp