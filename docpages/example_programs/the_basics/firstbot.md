\page firstbot Creating Your First Bot

In this example we will create a C++ version of the [discord.js](https://discord.js.org/#/) example program.

The two programs can be seen side by side below:

<table>
<tr>
<th>D++</th>
<th>Discord.js</td>
</tr>
<tr>
<td>


~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

const std::string    BOT_TOKEN    = "add your token here";

int main() {
    dpp::cluster bot(BOT_TOKEN);

    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([](const dpp::slashcommand_t& event) {
         if (event.command.get_command_name() == "ping") {
            event.reply("Pong!");
        }
    });

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            bot.global_command_create(
                dpp::slashcommand("ping", "Ping pong!", bot.me.id)
            );
        }
    });

    bot.start(dpp::st_wait);
}
~~~~~~~~~~~~~~~


</td>
<td>


~~~~~~~~~~~~~~~{.cpp}
let Discord = require('discord.js');


let BOT_TOKEN   = 'add your token here';


let bot = new Discord.Client({ intents: [] });


bot.on('interactionCreate', (interaction) => {
    if (interaction.isCommand() && interaction.commandName === 'ping') {
        interaction.reply({content: 'Pong!'});
    }
});


bot.once('ready', async () => {
    await client.commands.create({
        name: 'ping',
        description: "Ping pong!"
    });
});


bot.login(BOT_TOKEN);‍
~~~~~~~~~~~~~~~


</td>
</tr>
</table>

Let's break this program down step by step:

### 1. Start with an empty C++ program

Make sure to include the header file for the D++ library with the instruction \#include `<dpp/dpp.h>`!

~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main() {
}
~~~~~~~~~~~~~~

### 2. Create an instance of dpp::cluster

To make use of the library you must create a dpp::cluster object. This object is the main object in your program like the `Discord.Client` object in Discord.js.

You can instantiate this class as shown below. Remember to put your bot token in the constant!

~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

const std::string    BOT_TOKEN    = "add your token here";

int main() {
    dpp::cluster bot(BOT_TOKEN);
}
~~~~~~~~~~~~~~~

### 3. Attach to an event

To have a bot that does something, you should attach to some events. Let's start by attaching to the `on_ready` event (dpp::cluster::on_ready) which will notify your program when the bot is connected. In this event, we will register a slash
command called 'ping'. Note that we must wrap our registration of the command in a template called dpp::run_once which prevents it from being re-run
every time your bot does a full reconnection (e.g. if the connection fails).

~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

const std::string    BOT_TOKEN    = "add your token here";

int main() {
    dpp::cluster bot(BOT_TOKEN);

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            bot.global_command_create(dpp::slashcommand("ping", "Ping pong!", bot.me.id));
        }
    });
}
~~~~~~~~~~~~~~~~

### 4. Attach to another event to receive slash commands

If you want to handle a slash command, you should also attach your program to the `on_slashcommand` event (dpp::cluster::on_slashcommand) which is basically the same as the Discord.js `interactionCreate` event. Lets add this to the program before the `on_ready` event:

~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

const std::string    BOT_TOKEN    = "add your token here";

int main() {
    dpp::cluster bot(BOT_TOKEN);

    bot.on_slashcommand([](const dpp::slashcommand_t& event) {
    });

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            bot.global_command_create(dpp::slashcommand("ping", "Ping pong!", bot.me.id));
        }
    });
}
~~~~~~~~~~~~~~

### 5 . Add some content to the events

Attaching to an event is a good start, but to make a bot you should actually put some program code into the interaction event. We will add some code to the `on_slashcommand` to look for our slash command '/ping' and reply with `Pong!`:

~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

const std::string    BOT_TOKEN    = "add your token here";

int main() {
    dpp::cluster bot(BOT_TOKEN);

    bot.on_slashcommand([](const dpp::slashcommand_t& event) {
         if (event.command.get_command_name() == "ping") {
            event.reply("Pong!");
        }
    });

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            bot.global_command_create(dpp::slashcommand("ping", "Ping pong!", bot.me.id));
        }
    });

}
~~~~~~~~~~~~~~~~~~~~~~~

Let's break down the code in the `on_slashcommand` event so that we can discuss what it is doing:

~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    bot.on_slashcommand([](const dpp::slashcommand_t& event) {
         if (event.command.get_command_name() == "ping") {
            event.reply("Pong!");
        }
    });
~~~~~~~~~~~~~~~~~~~~~~~

This code is simply comparing the command name `event.command.get_command_name()` (dpp::interaction::get_command_name) against the value in a constant string value `"ping"`. If they match, then the `event.reply` method is called.

The `event.reply` function (dpp::slashcommand_t::reply) replies to a slash command with a message. There are many ways to call this function to send embed messages, upload files, and more, but for this simple demonstration we will just send some message text.

### 6. Add code to start the bot!

To make the bot start, we must call the dpp::cluster::start method, e.g. in our program by using `bot.start(dpp::st_wait)`.

We also add a line to tell the library to output all its log information to the console, `bot.on_log(dpp::utility::cout_logger());` - if you wanted to do something more advanced, you can replace this parameter with a lambda just like all other events.

The parameter which we set to false indicates if the function should return once all shards are created. Passing `false` here tells the program you do not need to do anything once `bot.start` is called.

~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

const std::string    BOT_TOKEN    = "add your token here";

int main() {
    dpp::cluster bot(BOT_TOKEN);

    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([](const dpp::slashcommand_t& event) {
         if (event.command.get_command_name() == "ping") {
            event.reply("Pong!");
        }
    });

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            bot.global_command_create(dpp::slashcommand("ping", "Ping pong!", bot.me.id));
        }
    });

    bot.start(dpp::st_wait);
}
~~~~~~~~~~~~~~

### 7. Compile and run your bot

Compile your bot using `g++ -std=c++17 -o bot bot.cpp -ldpp` (if your .cpp file is called `bot.cpp`) and run it with `./bot`.

### 8. Inviting your bot to your server

When you invite your bot, you must use the `applications.commands` and `bots` scopes to ensure your bot can create guild slash commands. For example:

`https://discord.com/oauth2/authorize?client_id=YOUR-BOTS-ID-HERE&scope=bot+applications.commands&permissions=BOT-PERMISSIONS-HERE`

Replace `YOUR-BOTS-ID-HERE` with your bot's user ID, and `BOT-PERMISSIONS-HERE` with the permissions your bot requires.

**Congratulations** - you now have a working bot written using the D++ library!

