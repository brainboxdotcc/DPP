# Frequently Asked Questions (FAQ)

[TOC] 

## Is this library in production use?
This library powers the bot [TriviaBot](https://triviabot.co.uk) which has over **151,000 servers**, and [Sporks](https://sporks.gg) which has over **3,500 severs**. The library's use in these bots shows that the library is production ready for bots of all sizes.

## How much RAM does this library use?
In production on TriviaBot, the bot takes approximately 2gb of ram to run 18 separate processes (this is approximately **140mb** per process) on a production bot with 36 million users and 151,000 guilds. Each process takes under 1% CPU. This is less than a quarter of the memory of a similar C++ Discord library, **Aegis.cpp** (version 2).

For a very small bot, you can get the memory usage as low as **6 megabytes** on a Raspberry Pi.

## How do I use this library in Windows?
The easiest way is to use our [template project](https://github.com/brainboxdotcc/windows-bot-template). If you are unable to do this, download the precompiled latest release from our GitHub releases, and take the dlls, `.lib` file, and header files (`bin`, `lib` and `include` directories), placing them in a easily accessible place on your computer. Go into Visual Studio project settings in a new project, and point the project directories (notably the library directories and and include directories) at the correct locations. Add the `include` folder you extracted to your include directories, and add `dpp.lib` to your library directories. Ensure the project is set to C++17 standard in the settings. You should then be able to compile example programs within that project. When you run the program you have compiled you must ensure that all the dll files from the `bin` directory exist in the same directory as your executable.

## Does this library support Visual Studio 2022?
Yes! The master branch comes with pre-built binaries for Visual Studio 2022 and our Windows bot template has a [vs2022 branch](https://github.com/brainboxdotcc/windows-bot-template/tree/vs2022) which you can clone to get Visual Studio 2022 specific code. For the time being we support both Visual Studio 2019 and 2022. At some point in the future only 2022 may be supported as 2019 becomes outdated.

## How much of the library is completed?
All REST calls (outbound commands) are completed including all currently available interactions, and all Discord events are available. The library also has voice support.

## How do I chat with the developers or get help?
The best place to do this is on the [Discord server](https://discord.gg/dpp). You most likely won't get an answer immediately (we have lives, and need to sleep sometimes), but we will be willing to help!

## How can I contribute to development?
Just star and fork a copy of the repository, and submit a Pull Request! We won't bite! Authors of accepted pull requests get a special role on our [Discord server](https://discord.gg/dpp).

## Whats the best way to learn C++?
A simple search can find some learning tools, however not all are good. Here is a list of some (good) learning resources:

* [CodeAcademy](https://www.codecademy.com/learn/c-plus-plus)
* [Learn CPP](https://www.learncpp.com/)
* [Learn CPP (Very Basic)](https://www.learn-cpp.org/)

If you don't understand something then feel free to ask in the [Discord server](https://discord.gg/dpp) ...*we don't bite!*

## Do I need to be an expert in C++ to use this library?
NO! Definitely not! We have tried to keep things as simple as possible. We only use language features where they make sense, not just because they exist. Take a look at the example program (`test.cpp` and you'll see just how simple it is to get up and running quickly). We use a small subset of C++17 and C++14 features.

## Why is D++ also called DPP
DPP is short for *D Plus Plus* (D++), a play on the Discord and C++ names. You'll see the library referred to as `dpp` within source code as `d++` is not a valid symbol so we couldn't exactly use that as our namespace name.

## Is D++ a single header library?
No, D++ is a classically designed library which installs itself to your library directory/system directory as a shared object or dll. You must link to its `.lib` file and include its header files to make use of it. We have no plans for a single-header build.

## Does this library support slash commands/interactions?
Yes! This library supports slash commands and interactions. For more information please see \ref slashcommands "Using Slash Commands and Interactions".

## Does this library support buttons/drop down menus (message components)?
Yes! This library supports button message components, e.g. interactive buttons on the bottom of messages. For more information please see our \ref components "Using component interactions" and \ref components2 "Using component interactions (advanced)" examples.

## Is the library asynchronous?
All functions within D++ are multi-threaded. You should still avoid doing long operations within event handlers or within callbacks, to prevent starvation of the threads managing each shard. Various blocking operations such as running files and making HTTP requests are offered as library functions (for example dpp::utility::exec)

## Does this library support voice?
Yes! This library supports voice and will automatically enable voice if your system has the libopus and libsodium libraries. When running `cmake` the script will identify if these libraries are found. See the example programs for information on how to send audio.

## Does this library support sharding?
Yes! D++ supports sharding and also clustering (grouping of shards into one process) to ensure it is scalable for small and large bots alike.

## How do I contribute to the documentation and website?
The documentation and website are built using Doxygen. To contribute to site pages, submit a Pull Request to the main repository. The site pages can be found within the `docpages` directory. Details of classes, variables, namespaces etc are auto generated from Doxygen comments within the library source code in the `include` and `src` folders.

## What version of the Discord API does this library support?
D++ only supports Discord API v10, the latest version. D++ major version numbers match the supported Discord API version.

## Does this Discord library support the threads feature?
Yes! D++ supports Discord threads. You can create, edit and delete threads and also attach events watching for messages within threads.

## Does D++ require C++20 support?
No, at the current time we do not use any C++20 features. Some C++17 features are used, which are available in all recent compilers.

## When I start my bot i get an error: "error while loading shared libraries: libdpp.so: cannot open shared object file: No such file or directory"
To fix this issue, run `ldconfig`: `sudo ldconfig` as root. Log out if your SSH session and log back in, and the bot should be able to find the library.

## When compiling with voice support, i get an error: "No rule to make target 'sodium_LIBRARY_DEBUG-NOTFOUND', needed by 'libdpp.so'. Stop."
The libsodium package requires pkg-config, but does not check for it when installed. Install it as root, e.g. `sudo apt install pkg-config`. Rerun cmake, and rebuild the library.

## When I try to instantiate a dpp::cluster in windows, a std::bad_alloc exception is thrown
If this happens, ensure you are using the correct precompiled build of the library. Our precompiled binaries are built in two forms, **release mode** and **debug mode** for Visual Studio 2019/2022. These two versions of the library are not cross-compatible due to differences in the debug and release libstdc++. You should not need to build your own copy, but please see the section about \ref buildwindows for more information on how to build your own copy, if needed.

## Does this library build/run on Raspberry Pi?
Yes! This project will build and run on Raspberry Pi and is very much suited to this kind of system. It may take some time (read: hours) to compile the project on your Raspberry Pi unless you build it using a cross compiler. We offer pre-built `.deb` files for arm6, arm7 and arm64, you should use these where possible to avoid having to compile it by hand, or you can use a cross-compiler to build it on your PC then transfer the compiled binaries across.

## There are so many versions! Which deb file do i need for my Raspberry Pi?
Depending on which Raspberry Pi version you have, you will need to download a different release binary:
<table>
<tr>
	<th>Raspberry Pi Model</th>
	<th>Deb file to install</th>
	<th>Arch</th>
</tr>
<tr><td>Raspberry Pi Zero/Zero W</td><td>`libdpp-x.x.x-linux-rpi-arm6.deb`</td><td>ARMv6</td></tr>
<tr><td>Raspberry Pi 3</td><td>`libdpp-x.x.x-linux-rpi-arm7hf.deb`</td><td>ARMv7HF</td></tr>
<tr><td>Raspberry Pi 4</td><td>`libdpp-x.x.x-linux-rpi-arm7hf.deb`</td><td>ARMv7HF</td></tr>
<tr><td>Raspberry Pi 4 with 64 Bit Linux</td><td>`libdpp-x.x.x-linux-rpi-arm64.deb`</td><td>ARM64</td></tr>
</table>

## Are other ARM devices supported?
Yes! We have confirmed that the D++ deb file will successfully install and operate on various forms of cellphone or non-pi ARM devices. If you get it working on any other devices please let us know and we can build a compatibility chart.

## Can I run a D++ bot in repl.it?
Yes! You can indeed run your bot in a repl.it container. [You can find a ready to go demo repl here](https://replit.com/@braindigitalis/dpp-demo-bot). We also have a [guide showing how to do this](https://dpp.dev/building-a-cpp-discord-bot-in-repl.html).

## Why do the "get" functions like "messages_get" return void rather than what I'm after?
All the functions that obtain data directly from Discord (as opposed to the cache) perform HTTPS requests and may have to wait, either for the request itself or for their turn in a queue to respect rate limits. As such, it does not make sense that they should return a value, as this would mean they block execution of your event. Instead, each has a lambda, a function handler which receives the result of the request, which you can then read from within that function to get the data you are interested in. Note that this result will arrive on a different thread to the one which made the request. If you instead want the function to return a value, use the methods ending with `_sync` that will block until they have a response. Note that these forms of call will throw an exception on failure.

## Can i use a user token with this library (as opposed to a bot token)?
No. This feature is not supported as it is against the Discord Terms Of Service, and therefore we have no plans to ever support it. You should not automate any user token. Some libraries used to support this but it is a legacy feature of those libraries (where still available) dating back to before Discord offered an official public API. Please be aware that if Discord ever catch you automating a user token (or making a user client that uses a bot token) they can and do ban people for this.
