# Welcome to the D++ developer wiki!

## What is D++ (DPP)?

<img src="DPP-Logo.png" align="right" style="max-width: 20% !important"/>
D++ is a lightweight and simple library for Discord written in modern C++. It is designed to cover as much of the API specification as possible and take very little 
memory to do so, even when caching large amounts of the data to cut down on HTTP requests.

It is created by the developer of [TriviaBot](https://triviabot.co.uk) and contributed to by a dedicated team of developers.

*This project is now in stable development and accepting PRs and feature requests -- Don't be a stranger! If you want to contribute, just get in touch via [github](https://github.com/brainboxdotcc/DPP) or our [Discord server](https://discord.gg/dpp)!*

## Library features:

* Really small memory footprint
* Efficient caching system for guilds, channels, guild members, roles, users
* Sharding (Many shards, one process: specify the number of shards, or let the library decide)
* [Slash Commands/Interactions support](https://dpp.brainbox.cc/slashcommands.html)
* [Voice support](https://dpp.brainbox.cc/soundboard.html)
* The entire Discord API is available for use in the library
* Stable [Windows support](https://dpp.brainbox.cc/buildwindows.html)
* Ready-made compiled packages for Windows, Raspberry Pi (ARM64/ARM7) and Debian x86/x64

## Supported Operating Systems

The library runs great on **Linux**. **Windows** is also supported and we offer ready made compiled DLL and LIB files for easy integration into any windows visual studio 2019 project.
**Mac OS X** is also functional and stable, as is running your bot on a **Raspberry Pi** - we offer a prebuilt .deb for ARM64, ARM6 and ARM7 to save on having to wait for it to compile.

The library may work fine in other operating systems too, if you run a D++ bot on something not listed here please let us know!

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

For a very small bot, you can get the memory usage as low as **6 megabytes** on a Raspberry Pi.

## How do I use this library in Windows?
The easiest way is to download the precompiled latest release from our github releases, and take the dlls, .lib file, and header files (`bin`, `lib` and `include` directories), put them in a place on your hard disk where you know where they are. Go into visual studio project settings in a new project, and point the project directories (notably the library directories and and include directories) at the correct locations. Add the `include` folder you extracted to your include directories, and add  `dpp.lib` to your library directories. Ensure the project is set to C++17 standard in the settings. You should then be able to compile example programs within that project. When you run the program you have compiled you must ensure that all the dll files from the `bin` directory exist in the same directory as your executable.

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
If this happens, ensure you are using the correct precompiled build of the library. Our precompiled binaries are built in two forms, release mode and debug mode, for x64 vs2019 only.
If you require a build for a newer visual studio, you will have to compile it yourself from the github sources. Please see the section about \ref buildwindows for more information on how to do this.

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

## Can i run a D++ bot in repl.it?
No, unfortunately Repl.it has too outdated a version of g++, clang and cmake to compile the library. Even if you could get it to build, chances are that it would exceed the permitted maximum run time and go to sleep before completing compilation of the library. Repl.it and similar services are designed more for experimentation with interpreted langauges such as javascript, and heavily team based development. If at a later date Repl.it directly include the library as a pre-built install on their docker images, this could change matters and you would be able to use it there.

## Why do the "get" functions like "messages_get" return void rather than what I'm after?
All the functions that obtain data directly from Discord (as opposed to the cache) perform HTTPS requests and may have to wait, either for the request itself or for their turn in a queue to respect rate limits. As such, it does not make sense that they should return a value, as this would mean they block execution of your event. Instead, each has a lambda, a function handler which receives the result of the request, which you can then read from within that function to get the data you are interested in. Note that this result will arrive on a different thread to the one which made the request.

