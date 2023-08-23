\page detecting-messages Listening to messages

Sometimes, you may want to listen out for a message, rather than a command. This could be used for many cases like a spam filter, a bot that would respond to movie quotes with gifs, or a chat bot! However, in this page, we'll be using this to create a moderation filter (detect bad words).

\warning As of August 30th, 2022, Discord made Message Content a privileged intent. Whilst this means you can still use prefixed messages as commands, Discord does not encourage this and heavily suggests you use [slash commands](/slashcommands.html). If you wish to create commands, use [slash commands](/slashcommands.html), not messages.

~~~~~~~~~~{.cpp}
#pragma once

#include <dpp/dpp.h>

class MessageListener {

public:

    /* Create a static function that can be called anywhere. */
    static void on_message_create(const dpp::message_create_t& event) {

        /* See if the message contains the phrase we want to check for.
         * If there's at least a single match, we reply and say it's not allowed.
         */
        if (event.msg.content.find("bad word") != std::string::npos) {
            event.reply("That is not allowed here. Please, mind your language!", true);
        }
    }

};
~~~~~~~~~~

If all went well, you should have something like this!

\image html badwordfilter.png