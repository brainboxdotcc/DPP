# Frequently Asked Questions (FAQ)

## How will you ensure this library is ready for production use?
We will be changing over our production bots [TriviaBot](https://triviabot.co.uk) which has 86,000 servers and 8.1 million users, and our second bot [Sporks](https://sporks.gg) which has 2600 servers to D++ from Aegis. The intent is for both these bots to be stable, so these will be the acid test to prove that the library is production ready for bots of all sizes.

## How much RAM does this library use?
During testing this library takes approximately 32 megabytes of ram to cache 250,000 users and 2200 guilds, with their respective roles and emojis. We will update these figures as we continue to test.

## How much of the library is completed?
All REST calls (outbound commands) are completed with the exception of *slash commands* and *audit log*, and all Discord events are available. We still need to add voice support, and we will update this as we progress.

## How do I chat with the developers or get help?
The best place to do this is on the [discord server](https://discord.gg/RnG32Ctyq7). You most likely won't get an answer immediately (we have lives, and need to sleep sometimes), so feel free to lurk and join the community!

## How can I contribute to development?
Just star and fork a copy of the repository, and submit a Pull Request! We won't bite! Authors of accepted pull requests get a special role on [the support server](https://discord.gg/RnG32Ctyq7).

## Whats the best way to learn C++?
A simple search can find some learning tools, however not all are good. Here is a list of learning resources:

* [CodeAcademy](https://www.codecademy.com/learn/c-plus-plus)
* [Learn CPP](https://www.learncpp.com/)
* [Learn CPP (Very Basic)](https://www.learn-cpp.org/)

If you don't understand something then feel free to ask in the [discord server](https://discord.gg/RnG32Ctyq7) ...*we don't bite!*

## Do I need to be an expert in C++ to use this library?
NO! Definitely not! We have tried to keep things as simple as possible. We only use language features where they make sense, not just because they exist. Take a look at the example program (`test.cpp` and you'll see just how simple it is to get up and running quickly). We use a small subset of C++17 and C++14 features.

## Why is D++ also called DPP
DPP is short for **D**iscord **P**lus **P**lus (D++), a play on the Discord and C++ names.

## Is D++ a single header library?
No, D++ is a classically designed library which installs itself to your library directory/system directory as a shared object or dll. You must link to its .lib file and include its header files to make use of it. We have no plans for a single-header build.

## Will this library support voice?
Eventually it is our intent to support voice using `libopus`. This will follow once the rest of the library is complete and will be a `cmake` flag, so that if you don't need it, you don't need to include it.

## Does this library support sharding?
Yes! D++ supports sharding and also clustering (grouping of shards into one process) to ensure it is scalable for small and large bots alike.

## How do I contribute to the documentation and website?
The documentation and website are built using Doxygen. To contribute to site pages, submit a Pull Request to the main repository. The site pages can be found within the `docpages` directory. Details of classes, variables, namespaces etc are auto generated from Doxygen comments within the library source code in the `include` and `src` folders.

## What version of the Discord API does this library support?
D++ Only supports Discord API v8, the latest version.
