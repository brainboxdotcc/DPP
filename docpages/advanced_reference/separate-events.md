\page separate-events Separating events into new classes

If you're someone that loves file organisation (or you hate how cluttered your `main.cpp` has become) then you may be interested in moving events into separate classes outside of the `main.cpp` file. This is a great way to improve readability and can be helpful in many cases! For example, you can have two classes on the same event, except one could be reading messages for spam and one could be reading messages for bad words!

In this tutorial, we'll be taking the [Listening to messages](/detecting-messages.html) example and moving the `on_message_create` event into a different class.

To get started, you can create a folder called `listeners` inside `src` (where your `main.cpp` is) if you'd like to put it there! We'll be doing exactly that so, if you'd like to stick along with the tutorial, get creating that folder!

Now, you can create a new header and cpp file in this folder! For this tutorial, we'll be naming both these files `message_listener`!

If you're using CMake, you'll need to add this to your `CMakeLists.txt`. Some IDEs automatically do this but it's always worth double-checking!

Once that's done, it should look similar to this (this screenshot has more files in, so it won't be identical!):

\image html file_example_listeners.png

First, we need to define the function that will be called when the event fires. We do this in the `message_listener.h`, like so:

~~~~~~~~~~{.cpp}
#pragma once

#include <dpp/dpp.h>

class message_listener {

public:

    /* Create a static function that can be called anywhere. */
    static void on_message_create(const dpp::message_create_t& event);

};
~~~~~~~~~~

Then we need to add our code for what should happen when this event fires. We do this in the `message_listener.cpp`, like so:

~~~~~~~~~~{.cpp}
#include "message_listener.h"

void message_listener::on_message_create(const dpp::message_create_t &event) {
    /* See if the message contains the phrase we want to check for.
     * If there's at least a single match, we reply and say it's not allowed.
     */
    if (event.msg.content.find("bad word") != std::string::npos) {
        event.reply("That is not allowed here. Please, mind your language!", true);
    }
}

~~~~~~~~~~

Now, you'll have a nice area where you can easily see the code, without scrolling through all of your `main.cpp` file just to get to this event!

However, we've not finished yet! If you thought "How does the `main.cpp` file actually know to call this?" then, 10 points to you! It doesn't know! We need to go do that now. So, let's do exactly that.

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include "listeners/message_listener.h" 

int main()
{
    /* Create the bot, but with our intents so we can use messages. */
	dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content);

	bot.on_log(dpp::utility::cout_logger());

    /* Fires our event that is located in MessageListener when the bot detects a message in any server and any channel it has access to. */
    bot.on_message_create(&message_listener::on_message_create);

	bot.start(dpp::st_wait);

	return 0;
}
~~~~~~~~~~

And there we go! How tidy is that?

Now, the possibilities to this are not limited. If you wish to do this twice (as I explained at first), you can simply have another class and just copy the `bot.on_message_create` line below in the `main.cpp` file and then you can change it to reference the second class, meaning you have two message events firing in two separate classes!