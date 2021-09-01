# Welcome to the D++ developer wiki!

## What is D++ (DPP)?

<img src="DPP-Logo.png" align="right" style="max-width: 20% !important"/>
D++ is a lightweight and simple library for Discord written in modern C++. It is designed to cover as much of the API specification as possible and take very little 
memory to do so, even when caching large amounts of the data to cut down on HTTP requests.

It is created by the developer of [TriviaBot](https://triviabot.co.uk) and contributed to by a dedicated team of developers.

This project is in beta stages of development.

## Library features:

* Really small memory footprint
* Efficient caching system for guilds, channels, guild members, roles, users
* Sharding (Many shards, one process: specify the number of shards, or let the library decide)
* Voice support
* Slash command/interaction support
* Pretty much the entire API is supported
* Windows support

Want to help? Drop me a line or send me a PR. I'll be choosy about what PRs i accept whilst the library is in such a heavy state of development.

## Supported Operating Systems

The library runs best on Linux. Windows is supported via cmake and Visual Studio 2019 but not encouraged for production use. The library may work fine in other operating systems too, but with no access to these we cannot support them.

## Getting started
* [GitHub Repository](https://github.com/brainboxdotcc/DPP)
* [Discord Server](https://discord.gg/dpp)
* \subpage frequently-asked-questions "Frequently Asked Questions"
* \ref buildlinux
* \ref buildwindows
* \ref buildosx

## Architecture
* \ref clusters-shards-guilds
* \ref thread-model

## Learning Resources
* [C++ for JavaScript Developers](https://pawelgrzybek.com/cpp-for-javascript-developers/)
* [C++ In Four Hours](https://www.youtube.com/watch?v=vLnPwxZdW4Y&vl=en)

\page frequently-asked-questions Frequently Asked Questions

# Frequently Asked Questions (FAQ)

## Is this library in production use?
This library powers the bot [TriviaBot](https://triviabot.co.uk) which has over **111,000 servers**, and [Sporks](https://sporks.gg) which has over **2,800 severs**. The library's use in these bots shows that the library is production ready for bots of all sizes.

## How much RAM does this library use?
In production on TriviaBot, the bot takes approximately 2gb of ram to run 18 separate processes (this is approximately **140mb** per process) on a production bot with 11 million users and 111,000 guilds. Each process takes under 1% CPU. This is less than a quarter of the memory of a similar C++ Discord library, **Aegis.cpp** (version 2).

## How much of the library is completed?
All REST calls (outbound commands) are completed including slash commands, and all Discord events are available. The library also has voice support.

## How do I chat with the developers or get help?
The best place to do this is on the [discord server](https://discord.gg/dpp). You most likely won't get an answer immediately (we have lives, and need to sleep sometimes), so feel free to lurk and join the community!

## How can I contribute to development?
Just star and fork a copy of the repository, and submit a Pull Request! We won't bite! Authors of accepted pull requests get a special role on [the support server](https://discord.gg/dpp).

## Whats the best way to learn C++?
A simple search can find some learning tools, however not all are good. Here is a list of learning resources:

* [CodeAcademy](https://www.codecademy.com/learn/c-plus-plus)
* [Learn CPP](https://www.learncpp.com/)
* [Learn CPP (Very Basic)](https://www.learn-cpp.org/)

If you don't understand something then feel free to ask in the [discord server](https://discord.gg/dpp) ...*we don't bite!*

## Do I need to be an expert in C++ to use this library?
NO! Definitely not! We have tried to keep things as simple as possible. We only use language features where they make sense, not just because they exist. Take a look at the example program (`test.cpp` and you'll see just how simple it is to get up and running quickly). We use a small subset of C++17 and C++14 features.

## Why is D++ also called DPP
DPP is short for *Discord Plus Plus* (D++), a play on the Discord and C++ names.

## Is D++ a single header library?
No, D++ is a classically designed library which installs itself to your library directory/system directory as a shared object or dll. You must link to its .lib file and include its header files to make use of it. We have no plans for a single-header build.

## Does this library support slash commands/interactions?
Yes! This library supports slash commands. For more information please see \ref slashcommands "Using Slash Commands and Interactions".

## Does this library support buttons (message components)?
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
D++ Only supports Discord API v9, the latest version.

## Does this Discord library support the v9 Threads feature?
Yes! D++ supports the new discord Threads. You can create, edit and delete threads and also attach events watching for messages within threads.

## Does D++ require C++20 support?
No, at present we do not use any C++20 features. Some C++17 features are used, which are available in all recent compilers.

## When I start my bot i get an error: "error while loading shared libraries: libdpp.so: cannot open shared object file: No such file or directory"
To fix this issue, as root run `ldconfig`: `sudo ldconfig`. Log out if your SSH session and log back in, and the bot should be able to find the library.

## When compiling with voice support, i get an error: "No rule to make target 'sodium_LIBRARY_DEBUG-NOTFOUND', needed by 'libdpp.so'. Stop."
The libsodium package requires pkg-config, but does not check for it when installed. Install it as root, e.g. `sudo apt install pkg-config`. Rerun cmake again, and rebuild the library.

## When using precompiled libraries in Windows, the program runs but is just a black console window and the bot doesnt come online?
If this happens, switch your project to release mode. Our precompiled binaries are built in release mode for x64 vs2019 only.
If you require a debug build, or a build for a newer visual studio, you will have to compile it yourself from the github sources.
Please see the section about \ref buildwindows for more information on how to do this.

## Does this library build on Raspberry Pi?
Yes! This project will build and run on Raspberry Pi and is very much suited to this kind of system. It may take some time (read: hours) to compile the project on your Raspberry Pi unless you build it using a cross compiler. We plan to offer pre-built `.deb` files for arm7 and arm64 soon, stay tuned for further updates!
