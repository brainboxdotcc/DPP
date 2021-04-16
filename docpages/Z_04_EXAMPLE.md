# Example Programs

The best way to experiment with these example programs is to delete the content from `test.cpp` in the library repository, and replace it with program code as shown below. You can then use `cmake` and `make` to build the bot without having to mess around with installation, dependencies etc.

* \subpage firstbot "Creating Your First Bot"
* \subpage slashcommands "Using Slash Commands and Interactions"
* \subpage soundboard "Creating a Sound Board"


\page firstbot Creating Your First Bot

In this example we will create a C++ version of the [discord.js](https://discord.js.org/#/) example program.

The two programs can be seen side by side below:

| C++/DPP               | JavaScript/Discord.js      |
|-----------------------|----------------------------|
| <img src="cprog.png" align="right" style="max-width: 100% !important"/> | <img src="jsprog.png" align="right" style="max-width: 100% !important"/> |

Let's break this program down step by step:

### 1. Start with an empty C++ program

Make sure to include the header file for the D++ library with the instruction `#include <dpp/dpp.h>`!

~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <iostream>

int main()
{
    return 0;
}
~~~~~~~~~~~~~~

### 2. Create an instance of dpp::cluster

To make use of the library you must create a dpp::cluster object. This object is the main object in your program like the `Discord.Client` object in Discord.js.

You can instantiate this class as shown below. Remember to put your bot token in the code where it says `"token"`!

~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <iostream>

int main()
{
    dpp::cluster bot("token");

    return 0;
}
~~~~~~~~~~~~~~~

### 3. Attach to an event

To have a bot that does something, you should attach to some events. Let's start by attaching to the `on_ready` event (dpp::cluster::on_ready) which will notify your program when the bot is connected:

~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <iostream>

int main()
{
    dpp::cluster bot("token");

    bot.on_ready([&bot](const dpp::ready_t & event) {
    });

    return 0;
}
~~~~~~~~~~~~~~~~

### 4. Attach to another event to reveice messages

If you want to receive messages, you should also attach your program to the `on_message_create` event (dpp::cluster::on_message_create) which is the same as the Discord.js `message` event. You add this to your program after the `on_ready` event:

~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <iostream>

int main()
{
    dpp::cluster bot("token");

    bot.on_ready([&bot](const dpp::ready_t & event) {
    });

    bot.on_message_create([&bot](const dpp::message_create_t & event) {
    });

    return 0;
}
~~~~~~~~~~~~~~

### 5 . Add some content to the events

Attaching to an event is a good start, but to make a bot you should actually put some program code into the events. Lets add some simple things into the events. We will add some code to the `on_ready` event to output the bot's nickname (dpp::cluster::me) and some code into the `on_message_create` to look for messages that are the text `!ping` and reply with `!pong`:

~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <iostream>

int main()
{
    dpp::cluster bot("token");

    bot.on_ready([&bot](const dpp::ready_t & event) {
        std::cout << "Logged in as " << bot.me.username << "!\n";
    });

    bot.on_message_create([&bot](const dpp::message_create_t & event) {
        if (event.msg->content == "!ping") {
            bot.message_create(dpp::message(event.msg->channel_id, "Pong!"));
        }
    });

    bot.start(false);
    return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~

Let's break down the code in the `on_message_create` event so that we can discuss what it is doing:

~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
if (event.msg->content == "!ping") {
	bot.message_create(dpp::message(event.msg->channel_id, "Pong!"));
}
~~~~~~~~~~~~~~~~~~~~~~~

This code is simply comparing the message content `event.msg->content` (dpp::message_create_t::content) against the value in a constant string value `"!ping"`. If they match, then the `message_create` function is called.

The `message_create` function (dpp::cluster::message_create) sends a message. There are many ways to call this function to send embed messages, upload files, and more, but for this simple demonstration we will just send some message text. The `message_create` function accepts a `dpp::message` object, which we create using two parameters:

* The channel ID to send to, which we get from `event.msg->channel_id` (dpp::message_create_t::channel_id)
* The message content, which for this demonstration we just make the static text `"Pong!"`.

### 6. Add code to start the bot!

To make the bot start, we must call the cluster::start method, e.g. in our program by using `bot.start(false)`.

The parameter which we set to false indicates if the function should return once all shards are created. Passing `false` here tells the program you do not need to do anything once `bot.start` is called, so the `return` statement directly afterwards is never reached:

~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <iostream>

int main()
{
    dpp::cluster bot("token");

    bot.on_ready([&bot](const dpp::ready_t & event) {
        std::cout << "Logged in as " << bot.me.username << "!\n";
    });

    bot.on_message_create([&bot](const dpp::message_create_t & event) {
        if (event.msg->content == "!ping") {
            bot.message_create(dpp::message(event.msg->channel_id, "Pong!"));
        }
    });

    bot.start(false);
    return 0;
}
~~~~~~~~~~~~~~

### 7. Run your bot

Compile your bot using `cmake ..` and `make` from the build directory, and run it with `./test` - Congratulations, you now have a working bot written using the D++ library!

\page soundboard Creating a Sound Board

This example script shows how to send a sound file to a voice channel.

~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <fmt/format.h>
#include <iomanip>
#include <sstream>

int main(int argc, char const *argv[])
{
	/* Load a sound file called Robot.pcm into memory.
	 * The bot expects PCM format, which are raw sound data,
	 * 2 channel stereo, 16 bit signed 48000Hz.
	 * 
	 * You can use audacity to export these from WAV or MP3 etc.
	 * 
	 * If you wanted to send a more complicated format, you could
	 * use a separate library to decode that audio to PCM. For
	 * example purposes, a raw PCM will suffice. This PCM file can
	 * be found within the bot's github repo.
	 */
	uint8_t* robot = nullptr;
	size_t robot_size = 0;
	std::ifstream input ("../testdata/Robot.pcm", std::ios::in|std::ios::binary|std::ios::ate);
	if (input.is_open()) {
		robot_size = input.tellg();
		robot = new uint8_t[robot_size];
		input.seekg (0, std::ios::beg);
		input.read ((char*)robot, robot_size);
		input.close();
	}

	/* Setup the bot */
	dpp::cluster bot("token");

	/* Use the on_message_create event to look for commands */
	bot.on_message_create([&bot, robot, robot_size](const dpp::message_create_t & event) {

		std::stringstream ss(event.msg->content);
		std::string command;

		ss >> command;

		/* Tell the bot to join a discord voice channel. Syntax: .join <channel id> */
		if (command == ".join") {
			dpp::snowflake channel_id;
			ss >> channel_id;
			if (channel_id) {
				dpp::DiscordClient* dc = bot.get_shard(0);
				dc->ConnectVoice(event.msg->guild_id, channel_id);
			}
		}

		/* Tell the bot to play the sound file 'Robot.pcm'. Syntax: .robot */
		if (command == ".robot") {
			/* This assumes that there is one shard and the voice channel's guild is is on it. 
			 * Only for demonstration purposes, DO NOT make this assumption in the real world!
			 */
			dpp::DiscordClient* dc = bot.get_shard(0);
			dpp::voiceconn* v = dc->GetVoice(event.msg->guild_id);
			if (v && v->voiceclient && v->voiceclient->IsReady()) {
				v->voiceclient->SendAudio((uint16_t*)robot, robot_size);
			}
		}
	});

	/* Start bot */
	bot.start(false);
	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~

\page slashcommands Using Slash Commands and Interactions

Slash commands and interactions are a newer feature of Discord which allow bot's commands to be registered centrally within the
system and for users to easily explore and get help with avaialble commands through the client itself.

To add a slash command you should use the dpp::cluster::global_command_create method for global commands (available to all guilds)
or dpp::cluster::guild_command_create to create a local command (available only to one guild).

When a user issues these commands the reply will arrive via the on_interaction_create event which you can hook, and take action
when you see your commands. It is possible to reply to an interaction by using either the dpp::interaction_create_t::reply method,
or by manually instantiating an object of type dpp::interaction_response and attaching a dpp::message object to it.

dpp::interaction_create_t::reply has two overloaded versions of the method, one of which accepts simple std::string replies, for
basic text-only messages (if your message is 'ephemeral' you must use this) and one which accepts a dpp::message for more advanced
replies. Please note that at present, Discord only supports a small subset of message and embed features within an interaction
response object.

\note A later version of the library will include a built in command handler system which will be able to integrate with slash commands
and interactions, making this process even more seamless.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <fmt/format.h>
#include <iomanip>

int main(int argc, char const *argv[])
{
	dpp::cluster bot("token");

	/* The interaction create event is fired when someone issues your commands */
	bot.on_interaction_create([&bot](const dpp::interaction_create_t & cmd) {
		/* Check which command they ran */
		if (cmd.command.data.name == "blep") {
			/* Fetch a parameter value from the command parameters */
			std::string animal = std::get<std::string>(cmd.get_parameter("animal"));
			/* Reply to the command. There is an overloaded version of this
			 * call that accepts a dpp::message so you can send embeds.
			 */
			cmd.reply(dpp::ir_channel_message_with_source, fmt::format("Blep! You chose {}", animal));
		}
	});

	bot.on_ready([&bot](const dpp::ready_t & event) {
		dpp::slashcommand newcommand;

		/* Create a new global command on ready event */
		newcommand
			.set_name("blep")
			.set_description("Send a random adorable animal photo")
			.set_application_id(bot.me.id);
			.add_option(
				dpp::command_option(dpp::co_string, "animal", "The type of animal", true).
					add_choice(dpp::command_option_choice("Dog", std::string("animal_dog"))).
					add_choice(dpp::command_option_choice("Cat", std::string("animal_cat"))).
					add_choice(dpp::command_option_choice("Penguin", std::string("animal_penguin")
				)
			)
		);

		/* Register the command */
		bot.global_command_create(newcommand, [&bot](const dpp::confirmation_callback_t & state));
	});

	bot.start(false);
	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~