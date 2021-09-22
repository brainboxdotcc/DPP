# Example Programs

The best way to experiment with these example programs is to delete the content from `test.cpp` in the library repository, and replace it with program code as shown below. You can then use `cmake` and `make` to build the bot without having to mess around with installation, dependencies etc.

* \subpage firstbot "Creating Your First Bot"
* \subpage slashcommands "Using Slash Commands and Interactions"
* \subpage soundboard "Creating a Sound Board"
* \subpage joinvc "Join or switch to the voice channel of the user issuing a command"
* \subpage spdlog "Integrating with spdlog"
* \subpage components "Using component interactions (buttons)"
* \subpage components3 "Using component interactions (select menus)"
* \subpage components2 "Using component interactions (advanced)"
* \subpage commandhandler "Using a command handler object"
* \subpage subcommands "Using sub-commands in slash commands"


\page firstbot Creating Your First Bot

In this example we will create a C++ version of the [discord.js](https://discord.js.org/#/) example program.

The two programs can be seen side by side below:

| C++/DPP               | JavaScript/Discord.js      |
|-----------------------|----------------------------|
| <img src="cprog.png" align="right" style="max-width: 100% !important"/> | <img src="jsprog.png" align="right" style="max-width: 100% !important"/> |

Let's break this program down step by step:

### 1. Start with an empty C++ program

Make sure to include the header file for the D++ library with the instruction \#include `<dpp/dpp.h>`!

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

This example script shows how to send a sound file to a voice channel. A few shortcuts are taken here, for more advanced techniques for connecting to a voice channel see the tutorial \ref joinvc

~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
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

		/* Tell the bot to join the discord voice channel the user is on. Syntax: .join */
		if (command == ".join") {
			dpp::guild * g = dpp::find_guild(event.msg->guild_id);
			if (!g->connect_member_voice(event.msg->author->id)) {
				bot.message_create(dpp::message(channel_id, "You don't seem to be on a voice channel! :("));
			}
		}

		/* Tell the bot to play the sound file 'Robot.pcm'. Syntax: .robot */
		if (command == ".robot") {
			dpp::voiceconn* v = event.from->get_voice(event.msg->guild_id);
			if (v && v->voiceclient && v->voiceclient->is_ready()) {
				v->voiceclient->send_audio((uint16_t*)robot, robot_size);
			}
		}
	});

	/* Start bot */
	bot.start(false);
	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~

\page joinvc Join or switch to the voice channel of the user issuing a command

When a user issues a command you may want to join their voice channel, e.g. in a music bot. If you are already on the same voice channel, the bot should do nothing (but be ready to instantly play audio) and if the user is on a different voice channel, the bot should switch to it. If the user is on no voice channel at all, this should be considered an error. This example shows how to do this.

\note Please be aware this example sends no audio, but indicates clearly in the comments where and how you should do so.

~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <iomanip>
#include <sstream>

int main(int argc, char const *argv[])
{
	/* Setup the bot */
	dpp::cluster bot("token");

	/* Use the on_message_create event to look for commands */
	bot.on_message_create([&bot, robot, robot_size](const dpp::message_create_t & event) {

		std::stringstream ss(event.msg->content);
		std::string command;

		ss >> command;

		/* Switch to or join the vc the user is on. Syntax: .join  */
		if (command == ".join") {
			dpp::guild * g = dpp::find_guild(event.msg->guild_id);
			auto current_vc = event.from->get_voice(event.msg->guild_id);
			bool join_vc = true;
			/* Check if we are currently on any vc */
			if (current_vc) {
				/* Find the channel id  that the user is currently on */
				auto users_vc = g->voice_members.find(event.msg->author->id);
				/* See if we currently share a channel with the user */
				if (users_vc != g->voice_members.end() && current_vc->channel_id == users_vc->second.channel_id) {
					join_vc = false;
					/* We are on this voice channel, at this point we can send any audio instantly to vc:

					 * current_vc->send_audio(...)
					 */
				} else {
					/* We are on a different voice channel. Leave it, then join the new one 
					 * by falling through to the join_vc branch below.
					 */
					event.from->disconnect_voice(event.msg->guild_id);
					join_vc = true;
				}
			}
			/* If we need to join a vc at all, join it here if join_vc == true */
			if (join_vc) {
				if (!g->connect_member_voice(event.msg->author->id)) {
					/* The user issuing the command is not on any voice channel, we can't do anything */
					bot.message_create(dpp::message(channel_id, "You don't seem to be on a voice channel! :("));
				} else {
					/* We are now connecting to a vc. Wait for on_voice_ready 
					 * event, and then send the audio within that event:
					 * 
					 * event.voice_client->send_audio(...);
					 * 
					 * NOTE: We can't instantly send audio, as we have to wait for
					 * the connection to the voice server to be established!
					 */
				}
			}
		}
	});

	/* Start bot */
	bot.start(false);
	return 0;
}

~~~~~~~~~~~~~~~~~~~~~~~~~

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

\note You can also use the unified command handler, which lets you combine channel based message commands and slash commands under the same lambda with the same code like they were one and the same. Note that after April of 2022 Discord will be discouraging bots from using commands that are prefixed messages via means of a privileged message intent. It is advised that you exclusively use slash commands, or the unified handler with only a prefix of "/" going forward for any new bots you create and look to migrating existing bots to this setup.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <dpp/fmt/format.h>

int main()
{
	dpp::cluster bot("token");

	/* The interaction create event is fired when someone issues your commands */
	bot.on_interaction_create([&bot](const dpp::interaction_create_t & event) {
		if (event.command.type == dpp::it_application_command) {
			dpp::command_interaction cmd_data = std::get<dpp::command_interaction>(event.command.data);
			/* Check which command they ran */
			if (cmd_data.name == "blep") {
				/* Fetch a parameter value from the command parameters */
				std::string animal = std::get<std::string>(event.get_parameter("animal"));
				/* Reply to the command. There is an overloaded version of this
				* call that accepts a dpp::message so you can send embeds.
				*/
				event.reply(dpp::ir_channel_message_with_source, fmt::format("Blep! You chose {}", animal));
			}
		}
	});

	bot.on_ready([&bot](const dpp::ready_t & event) {

		dpp::slashcommand newcommand;
		/* Create a new global command on ready event */
		newcommand.set_name("blep")
			.set_description("Send a random adorable animal photo")
			.set_application_id(bot.me.id)
			.add_option(
				dpp::command_option(dpp::co_string, "animal", "The type of animal", true).
					add_choice(dpp::command_option_choice("Dog", std::string("animal_dog"))).
					add_choice(dpp::command_option_choice("Cat", std::string("animal_cat"))).
					add_choice(dpp::command_option_choice("Penguin", std::string("animal_penguin")
				)
			)
		);

		/* Register the command */
		bot.global_command_create(newcommand);
	});

	bot.start(false);

	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

\page spdlog Integrating with spdlog

If you want to make your bot use spdlog, like aegis does, you can attach it to the on_log event. You can do this as follows:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <dpp/fmt/format.h>
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <iomanip>

int main(int argc, char const *argv[])
{
	dpp::cluster bot("token");

	const std::string log_name = "mybot.log";

	/* Set up spdlog logger */
	std::shared_ptr<spdlog::logger> log;
	spdlog::init_thread_pool(8192, 2);
	std::vector<spdlog::sink_ptr> sinks;
	auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
	auto rotating = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_name, 1024 * 1024 * 5, 10);
	sinks.push_back(stdout_sink);
	sinks.push_back(rotating);
	log = std::make_shared<spdlog::async_logger>("logs", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
	spdlog::register_logger(log);
	log->set_pattern("%^%Y-%m-%d %H:%M:%S.%e [%L] [th#%t]%$ : %v");
	log->set_level(spdlog::level::level_enum::debug);

	/* Integrate spdlog logger to D++ log events */
	bot.on_log([&bot, &log](const dpp::log_t & event) {
		switch (event.severity) {
			case dpp::ll_trace:
				log->trace("{}", event.message);
			break;
			case dpp::ll_debug:
				log->debug("{}", event.message);
			break;
			case dpp::ll_info:
				log->info("{}", event.message);
			break;
			case dpp::ll_warning:
				log->warn("{}", event.message);
			break;
			case dpp::ll_error:
				log->error("{}", event.message);
			break;
			case dpp::ll_critical:
			default:
				log->critical("{}", event.message);
			break;
		}
	});

	/* Add the rest of your events */

	bot.start(false);
	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

\page components Using component interactions (buttons)

Discord's newest features support sending buttons alongside messages, which when clicked by the user trigger an interaction which is routed by
D++ as an on_button_click event. To make use of this, use code as in this example.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <iostream>
#include <dpp/message.h>

int main()
{
	dpp::cluster bot("token");

	/* Message handler to look for a command called !button */
	bot.on_message_create([&bot](const dpp::message_create_t & event) {
		if (event.msg->content == "!button") {
			/* Create a message containing an action row, and a button within the action row. */
			bot.message_create(
				dpp::message(event.msg->channel_id, "this text has buttons").add_component(
					dpp::component().add_component(
						dpp::component().set_label("Click me!").
						set_emoji("😄").
						set_style(dpp::cos_danger).
						set_id("myid")
					)
				)
			);
		}
	});

	/* When a user clicks your button, the on_button_click event will fire,
	 * containing the custom_id you defined in your button.
	 */
	bot.on_button_click([&bot](const dpp::button_click_t & event) {
		/* Button clicks are still interactions, and must be replied to in some form to
		 * prevent the "this interaction has failed" message from Discord to the user.
 		 */
		event.reply(dpp::ir_channel_message_with_source, "You clicked: " + event.custom_id);
	});

	bot.start(false);

	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When the feature is functioning, the code below will produce buttons on the reply message like in the image below:

\image html button.png

\page commandhandler Using a command handler object

If you have many commands in your bot, and want to handle commands from multiple sources (for example modern slash commands, and more regular
prefixed channel messages) you should consider instantiating a dpp::commandhandler object. This object can be used to automatically route
commands and their parameters to functions in your program. A simple example of using this object to route commands is shown below, and will
route both the /ping (global slash command) and .ping (prefixed channel message command) to a lambda where a reply can be generated.

\note	This example automatically hooks the dpp::cluster::on_message and dpp::cluster::on_interaction_create events. This can be overridden
	if needed to allow you to still make use of these functions for your own code, if you need to do this please see the constructor
	documentation for dpp::commandhandler.

Note that because the dpp::commandhandler::add_command method accepts a std::function as the command handler, you may point a command handler
at a simple lambda (as shown in this example), a function pointer, or an instantiated class method of an object. This is extremely flexible
and allows you to decide how and where commands should be routed, either to an object oriented system or to a lambda based system.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main()
{
	dpp::cluster bot("token");

	/* Create command handler, and specify prefixes */
	dpp::commandhandler command_handler(&bot);
	/* Specifying a prefix of "/" tells the command handler it should also expect slash commands */
	command_handler.add_prefix(".").add_prefix("/");

	bot.on_ready([&command_handler](const dpp::ready_t &event) {

		command_handler.add_command(
			/* Command name */
			"ping",

			/* Parameters */
			{
				{"testparameter", dpp::param_info(dpp::pt_string, true, "Optional test parameter") }
			},

			/* Command handler */
			[&command_handler](const std::string& command, const dpp::parameter_list_t& parameters, dpp::command_source src) {
				std::string got_param;
				if (!parameters.empty()) {
					got_param = std::get<std::string>(parameters[0].second);
				}
				command_handler.reply(dpp::message("Pong! -> " + got_param), src);
			},

			/* Command description */
			"A test ping command",

			/* Guild id (omit for a global command) */
			819556414099554344
		);

	});


	bot.start(false);
	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

\page components3 Using component interactions (select menus)

This example demonstrates receiving select menu clicks and sending response messages.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

using json = nlohmann::json;

int main()
{

	dpp::cluster bot("token");

	/* Message handler to look for a command called !select */
	bot.on_message_create([&bot](const dpp::message_create_t & event) {
		if (event.msg->content == "!select") {
			/* Create a message containing an action row, and a select menu within the action row. */
			dpp::message m(event.msg->channel_id, "this text has a select menu");
			m.add_component(
				dpp::component().add_component(
					dpp::component().set_type(dpp::cot_selectmenu).
					set_placeholder("Pick something").
					add_select_option(dpp::select_option("label1","value1","description1").set_emoji("😄")).
					add_select_option(dpp::select_option("label2","value2","description2").set_emoji("🙂")).
					set_id("myselid")
				)
			);
			bot.message_create(m);
		}
	});
	/* When a user clicks your select menu , the on_select_click event will fire,
	 * containing the custom_id you defined in your select menu.
	 */
	bot.on_select_click([&bot](const dpp::select_click_t & event) {
		/* Select clicks are still interactions, and must be replied to in some form to
		 * prevent the "this interaction has failed" message from Discord to the user.
		 */
		event.reply(dpp::ir_channel_message_with_source, "You clicked " + event.custom_id + " and chose: " + event.values[0]);
	});

	bot.on_log([](const dpp::log_t & event) {
		if (event.severity > dpp::ll_trace) {
			std::cout << event.message << "\n";
		}
	});

	bot.start(false);

	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

\page components2 Using component interactions (advanced)

This example demonstrates receiving button clicks and sending response messages.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

using json = nlohmann::json;

int main()
{

	dpp::cluster bot("token");

	bot.on_button_click([&bot](const dpp::button_click_t & event) {
		if (event.custom_id == "10") {
			event.reply(dpp::ir_channel_message_with_source, dpp::message("Correct").set_flags(dpp::m_ephemeral));
		} else {
			event.reply(dpp::ir_channel_message_with_source, dpp::message("Incorrect").set_flags(dpp::m_ephemeral));
		}
	});

	bot.on_message_create([&bot](const dpp::message_create_t & event) {
		if (event.msg->content == "!ping2") {
			bot.message_create(
				dpp::message(event.msg->channel_id, "What is 5+5?").add_component(
					dpp::component().add_component(
						dpp::component().set_label("9").
						set_style(dpp::cos_primary).
						set_id("9")
					).add_component(
						dpp::component().set_label("10").
						set_style(dpp::cos_primary).
						set_id("10")
					).add_component(
						dpp::component().set_label("11").
						set_style(dpp::cos_primary).
						set_id("11")
					)
				)
			);
		}
	});

	bot.start(false);

	return 0;
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This code will send a different message for correct and incorrect answers.

\image html button_2.png

\page subcommands Using sub-commands in slash commands

This is how to use Subcommands within your Slash Commands for your bots.

To make a subcomamnd within your command use this
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>
#include <dpp/fmt/format.h>
#include <iostream>

int main() {

	/* Setup the bot */
	dpp::cluster bot("token");
	
	/* Executes on ready. */
	bot.on_ready([&bot](const dpp::ready_t & event) {
	// Define a slash command.
	dpp::slashcommand image;
	    image.set_name("image");
	    image.set_description("Send a specific image.");
	    image.add_option(
			// Create a subcommand type option.
 	       	dpp::command_option(dpp::co_sub_command, "dog", "Send a picture of a dog.").
				add_option(dpp::command_option(dpp::co_user, "user", "User to make a dog off.", false))
			);
		image.add_option(
			// Create another subcommand type option.
	        dpp::command_option(dpp::co_sub_command, "cat", "Send a picture of a cat.").
				add_option(dpp::command_option(dpp::co_user, "user", "User to make a cat off.", false))
			);
	// Create command with a callback.
	bot.global_command_create(image, [ & ]( const dpp::confirmation_callback_t &callback ) {
	                    if(callback.is_error()) {
	                        std::cout << callback.http_info.body <<  "\n" ;
	                    }
		});
	});

	/* Use the on_interaction_create event to look for commands */
	bot.on_interaction_create([&bot](const dpp::interaction_create_t & event) {
    	    if (event.command.type == dpp::it_application_command) {
        	    dpp::command_interaction cmd_data = std::get<dpp::command_interaction>(event.command.data);

				/* Check if the command is the image command. */
				if(cmd_data.name == "image") {
					/* Check if the subcommand is "dog" */
					if(cmd_data.options[0].name == "dog") {	
						/* Checks if the subcommand has any options. */
						if(cmd_data.options[0].options.size() > 0) {
							/* Get the user option as a snowflake. */
							dpp::snowflake user = std::get<dpp::snowflake>(cmd_data.options[0].options[0].value);
							event.reply(dpp::ir_channel_message_with_source, fmt::format("<@{}> has now been turned into a dog.", user)); 
						} else {
						/* Reply if there were no options.. */
						event.reply(dpp::ir_channel_message_with_source, "<A picture of a dog.>");
						}
					}
					/* Check if the subcommand is "cat" */
					if(cmd_data.options[0].name == "cat") {
						/* Checks if the subcommand has any options. */
						if(cmd_data.options[0].options.size() > 0) {
							/* Get the user option as a snowflake. */
							dpp::snowflake user = std::get<dpp::snowflake>(cmd_data.options[0].options[0].value);
							event.reply(dpp::ir_channel_message_with_source, fmt::format("<@{}> has now been turned into a cat.", user));
						} else {
						/* Reply if there were no options.. */
						event.reply(dpp::ir_channel_message_with_source, "<A picture of a cat.>");
						}
					}
				}
			}
	});

	bot.start(false);
    return 0;
} 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
