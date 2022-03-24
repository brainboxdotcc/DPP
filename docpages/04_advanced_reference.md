# Advanced Reference

* \subpage clusters-shards-guilds "Clusters, Shards and Guilds"
* \subpage thread-model "Thread Model"
* \subpage coding-standards "Coding Style Standards"
* \subpage unit-tests "Unit Tests"
* \subpage lambdas-and-locals "Ownership of local variables and safely transferring into a lambda"

\page clusters-shards-guilds Clusters, Shards and Guilds

D++ takes a three-tiered highly scalable approach to bots, with three levels known as Clusters, Shards and Guilds as documented below.

## Clusters

A bot may be made of one or more clusters. Each cluster maintains a queue of commands waiting to be sent to Discord, a queue of replies from Discord for all commands executed, and zero or more **shards**.

Small bots will require just one cluster. Clusters may be started within separate processes if required (this is recommended) and will split the required number of shards equally across themselves. There is some level of communication between clusters, however they all remain independent without any central "controller" process.

## Shards

A cluster contains zero or more shards. Each shard maintains a persistent websocket connection to Discord which receives all events the bot is made aware of, e.g. messages, channel edits, etc. 

Small bots require only one shard. You may however create as many as you wish. Once you reach 2500 guilds, Discord require that your bot have at least one shard for every 2500 guilds. Any shard which would have over 2500 guilds on it will be disallowed connection.

## Guilds

Guilds are what servers are known as to the API. There can be up to **2500** of these per shard. Each of these is usually a distinct community and will contain a collection of guild members (users with a membership of this guild), channels and roles, emojis and various other pieces of data.

\page thread-model Thread Model

\image html DPP_Architecture.svg

\page coding-standards Coding Style Standards

This page lists the coding style we stick to when maintaining the D++ library. If you are submitting a pull request or other code contribution to the library, you should stick to the styles listed below. If something is not covered here, ask on the [official discord server](https://discord.gg/dpp)!

## Class names, function names and method names
All class, variable/member, function and method names should use `snake_case`, similar to the style of the C++ standard library.

## Enums
Enums and their values should be `snake_case` as with class, function and method names. You do not need to use `enum class`, so make sure that enum values are prefixed with a prefix to make them unique and grouped within the IDE, e.g. `ll_debug`, `ll_trace` etc.


## Curly Braces, Brackets etc

Open curly braces on the same line as the keyword, for example:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
if (a == b) {
	c();
}
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use a space after the comma in parameter lists, and after opening brackets and before closing brackets except when calling a function, e.g.:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
std::vector<std::string> clowns = { "pennywise", "bobo" };

evaluate_clown(clowns[0], evilness(2.5, factor));
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Indentation
Indentation should always be tab characters. It is up to you how wide you set tab characters in your editor for your personal tastes. All code blocks delimited within curly braces should be indented neatly and uniformly.

## Constants and \#define macros
Constants and macros should be all `UPPERCASE` with `SNAKE_CASE` to separate words. Macros should not have any unexpected side effects.

## Comments
All comments should be in `doxygen` format (similar to javadoc). Please see existing class definitions for an example. You should use doxygen style comments in a class definition inside a header file, and can use any other comment types within the .cpp file. Be liberal with comments, especially if your code makes any assumptions!

## Symbol exporting
If you export a class which is to be accessible to users, be sure to prefix it with the `DPP_EXPORT` macro, for example:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
class DPP_EXPORT my_new_class {
public:
	int hats;
	int clowns;
};
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The `DPP_EXPORT` macro ensures that on certain platforms (notably Windows) the symbol is exported to be available to the library user.

## Public vs private vs protected
It is a design philosophy of D++ that everything possible in a class should be public, unless the user really does not need it (you should consider justifying in comments why) or user adjustment of the variable could badly break the functioning of the library. Avoid the use of accessors for setting/getting values in a class, except for bit fields, where you should provide accessors for setting and getting individual bits (for example, see `user.h`), or in the event you want to provide a "fluent" interface. The exception to this is where you want to provide a logic validation of a field, for example if you have a string field with a minimum and maximum length, you can provide a setter the user can *optionally use* which will validate their input.

## Exceptions
All exceptions thrown should derive from dpp::exception (see dpp/exception.h) - when validating string lengths, a string which is too long should be truncated using dpp::utility::utf8substr and any strings that are too short should throw a dpp::length_exception.

## Inheritance
Keep levels of inheritance low. If you need to inherit more than 3 levels deep, it is probable that the design could be simplified. Remember that at scale, there can be tens of millions of certain classes and each level of virtual nesting adds to the `vtable` of that object's instance in RAM.

## Bit field packing
Where discord provides boolean flags, if the user is expected to store many of the object in RAM, or in cache, you should pack all these booleans into bit fields (see `user.h` and `channel.h` for examples). In the event that the object is transient, such as an interaction or a message, packing the data into bit fields is counter intuitive. Remember that you should provide specific accessors for bit field values!

## Keep dependencies internal!
Where you are making use of an external dependency such as `opus` or `libssl`, do not place references to the types/structs, or the header files of these external libraries within the header files of D++. Doing so adds that library as a public dependency to the project (which is bad!). Instead make an opaque class, and/or forward-declare the structs (for examples see `sslclient.h` and `discordvoiceclient.h`).

## API type names
Where discord provide a name in PascalCase we should stick as closely to that name as possible but convert it to `snake_case`. For example, GuildMember would become `guild_member`.

## Don't introduce any platform-specific code
Do not introduce platform specific (e.g. windows only) code or libc functions. If you really must use these functions safely wrap them e.g. in `#ifdef _WIN32` and provide a cross-platform alternative so that it works for everyone.

## Select the right size type for numeric types
If a value will only hold values up to 255, use `uint8_t`. If a value cannot hold over 65536, use `uint16_t`. These types can help use a lot less ram at scale.

## Fluent design
Where possible, if you are adding methods to a class you should consider fluent design. Fluent design is the use of class methods tha return a reference to self (via `return *this`), so that you can chain object method calls together (in the way `dpp::message` and `dpp::embed` do). For example:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
class DPP_EXPORT my_new_class {
public:
	int hats;
	int clowns;

	my_new_class& set_hats(int new_hats);
	my_new_class& set_clowns(int new_clowns);
};

my_new_class& my_new_class::set_hats(int new_hats) {
	hats = new_hats;
	return *this;
}

my_new_class& my_new_class::set_clowns(int new_clowns) {
	clowns = new_clowns;
	return *this;
}

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This would allow the user to do this:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
dpp::my_new_class nc;
nc.set_hats(3).set_clowns(9001);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

## Keep all D++ related types in the dpp namespace

All types for the library should be within the `dpp` namespace. There are a couple of additional namespaces, e.g. `dpp::utility` for static standalone helper functions and helper classes, and `dpp::events` for internal websocket event handlers.

## Commit messages and Git

All pull requests ("PRs") should be submitted against the `dev` branch in GitHub. It’s good to have descriptive commit messages, or PR titles so that other contributors can understand about your commit or the PR Created. Read [conventional commits](https://www.conventionalcommits.org/en/v1.0.0-beta.3/) for information on how we like to format commit messages.

All PRs must pass the [GitHub Actions](https://github.com/brainboxdotcc/DPP/actions) tests before being allowed to be merged. This is to ensure that no code committed into the project fails to compile on any of our officially supported platforms or architectures.

\page unit-tests Unit Tests

## Running Unit Tests

If you are adding functionality to DPP, make sure to run unit tests. This makes sure that the changes do not break anything. All pull requests must pass all unit tests before merging.

Before running test cases, create a test server for your test bot. You should:

* Make sure that the server only has you and your test bot, and no one else
* Give your bot the administrator permission
* Enable community for the server
* Make an event
* Create at least one voice channel
* Create at least one text channel

Then, set the following variables to the appropriate values. (This uses a fake token, don't bother trying to use it.)

    export DPP_UNIT_TEST_TOKEN="ODI2ZSQ4CFYyMzgxUzkzzACy.HPL5PA.9qKR4uh8po63-pjYVrPAvQQO4ln"
    export TEST_GUILD_ID="907951970017480704"
    export TEST_TEXT_CHANNEL_ID="907951970017480707"
    export TEST_VC_ID="907951970017480708"
    export TEST_USER_ID="826535422381391913"
    export TEST_EVENT_ID="909928577951203360"

Then, after cloning and building DPP, run `./build/test` for unit test cases. 

\page lambdas-and-locals Ownership of local variables and safely transferring into a lambda

If you are reading this page, you have likely been sent here by someone helping you diagnose why your bot is crashing or why seemingly invalid values are being passed into lambdas within your program that uses D++.

It is important to remember that when you put a lambda callback onto a function in D++, that this lambda will execute at some point in the **future**. As with all things in the future and as 80s Sci Fi movies will tell you, when you reach the future things may well have changed!

\image html delorean-time-travel.gif

To explain this situation and how it causes issues i'd like you to imagine the age old magic trick, where a magician sets a fine table full of cutlery, pots, pans and wine. He indicates to the audience that this is authentic, then with a whip of his wrist, he whips the tablecloth away, leaving the cutlery and other tableware in place (if he is any good as a magician!)

Now imagine the following code scenario. We will describe this code scenario as the magic trick above, in the steps below:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
bot.on_message_create([&bot](const dpp::message_create_t & event) {
	int myvar = 0;
	bot.message_create(dpp::message(event.msg.channel_id, "foobar"), [&](const auto & cc) {
		myvar = 42;
	});
});
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In this scenario, the outer event, `on_message_create` is your tablecloth. The lambda inside the `bot.message_create` is the tableware and cutlery. The following chain of events happens in this code:

* The magician executes his magic trick (D++ the `bot.on_message_create entering` the outer lambda)
* Your code executes `bot.message_create()` inside this outer lambda
* D++ inserts your request to send a message into its queue, in another thread. The inner lambda, where you might later set `myvar = 42` is safely copied into the queue for later calling.
* The tablecloth is whipped away... in other words, `bot.on_message_create` ends, and all local variables including `myvar` become invalid
* At a later time (usually 80ms through to anything up to 4 seconds depending on rate limits!) the message is sent, and your inner lambda which was saved at the earlier step is called.
* Your inner lambda attempts to set `myvar` to 42... but `myvar` no longer exists, as the outer lambda has been destroyed....
* The table wobbles... the cutlery shakes... and...
* Best case scenario: you access invalid RAM no longer owned by your program by trying to write to `myvar`, and [your bot outright crashes horribly](https://www.youtube.com/watch?v=sm8qb2kP-fQ)!
* Worse case scenario: you silently corrupt ram and end up spending days trying to track down a bug that subtly breaks your bot...

The situation i am trying to describe here is one of object and variable ownership. When you call a lambda, **always assume that every non global reference outside of that lambda will be invalid when the lambda is called**! For any non-global variable always take a **copy** of the variable (not reference, or pointer). Global variables or those declared directly in `main()` are safe to pass as references.

For example, if we were to fix the broken code above, we could rewrite it like this:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
bot.on_message_create([&bot](const dpp::message_create_t & event) {
	int myvar = 0;
	bot.message_create(dpp::message(event.msg.channel_id, "foobar"), [myvar](const auto & cc) {
		myvar = 42;
	});
	std::cout << "here\n";
});
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Note however that when you set myvar within the inner lambda, this does **not effect** the value of the var outside it. Lambdas should be considered self-contained silos, and as they execute in other threads should not be relied upon to set anything that exists **outside of that lambda**.

\warning Always avoid just using `[&]` in a lambda to access all in the scope above. It is unlikely that half of this scope will still even be valid by the time you get a look at it!

Similarly, and important to note, your program **will not wait for bot.message_create to send its message and call its lambda** before continuing on to print `here`. It will instantly insert the request into its queue and bail straight back out (see the steps above) and immediately print the text.

If you do want to get variables out of your lambda, create a class, or call a separate function, and pass what you need into that function from the lambda **by value** or alternatively, you can use `std::bind` to bind a lambda directly to an object's method instead (this is great for modular bots).

If you are stuck, as this is a complex subject please do feel free to ask on the [official support server](https://discord.gg/dpp)!
