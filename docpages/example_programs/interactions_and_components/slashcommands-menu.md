\page slashcommands-menu Slash commands

Here, you will find examples on how to use slash commands, subcommands, and different types or parameters.

* \subpage slashcommands "Using Slash Commands"
* \subpage clearing_slashcommands
* \subpage subcommands "Slash command sub-commands"
* \subpage application-command-autocomplete "Slash command auto completion"
* \subpage discord-application-command-file-upload "Using file parameters in slash commands"
* \subpage commandhandler "Unified message/slash command handler"

\note Keep in mind, a slash command when sent to your bot is <b>NOT SENT AS A MESSAGE</b>! It is not a string but an interaction structure with its own data, and \ref resolved-objects "command parameters are objects".
