# Frequently Asked Questions (FAQ)

## Is this library in production use?
This library powers the bot [TriviaBot](https://triviabot.co.uk) which has over **92,000 servers**, and [Sporks](https://sporks.gg) which has over **2,600 severs**. The library's use in these bots shows that the library is production ready for bots of all sizes.

## How much RAM does this library use?
During testing this library takes approximately 400 megabytes of ram to cache 1,000,000 (1 million) users and 7700 guilds, with their respective roles and emojis, in production. This is half the memory of a similar C++ Discord library, **Aegis.cpp** (version 2).

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

## Is the library asynchronous?
All functions within D++ are multi-threaded. You should still avoid doing long operations within event handlers or within callbacks, to prevent starvation of the threads managing each shard. Various blocking operations such as running files and making HTTP requests are offered as library functions (for example dpp::utility::exec)

## Does this library support voice?
Yes! This library supports voice and will automatically enable voice if your system has the libopus and libsodium libraries. When running `cmake` the script will identify if these libraries are found. See the example programs for information on how to send audio.

## Does this library support sharding?
Yes! D++ supports sharding and also clustering (grouping of shards into one process) to ensure it is scalable for small and large bots alike.

## How do I contribute to the documentation and website?
The documentation and website are built using Doxygen. To contribute to site pages, submit a Pull Request to the main repository. The site pages can be found within the `docpages` directory. Details of classes, variables, namespaces etc are auto generated from Doxygen comments within the library source code in the `include` and `src` folders.

## What version of the Discord API does this library support?
D++ Only supports Discord API v8, the latest version.

## Does D++ require C++20 support?
No, at present we do not use any C++20 features. Some C++17 features are used, which are available in all recent compilers.

## When I start my bot i get an error: "error while loading shared libraries: libdpp.so: cannot open shared object file: No such file or directory"
To fix this issue, as root run `ldconfig`: `sudo ldconfig`. Log out if your SSH session and log back in, and the bot should be able to find the library.

## When compiling with voice support, i get an error: "No rule to make target 'sodium_LIBRARY_DEBUG-NOTFOUND', needed by 'libdpp.so'. Stop."
The libsodium package requires pkg-config, but does not check for it when installed. Install it as root, e.g. `sudo apt install pkg-config`. Rerun cmake again, and rebuild the library.