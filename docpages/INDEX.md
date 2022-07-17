# D++: A C++ Discord API Library for Bots

## What is D++ (DPP)?

D++ is a lightweight and simple library for Discord written in modern C++. It is designed to cover as much of the API specification as possible and to have a incredibly small memory footprint, even when caching large amounts of data.

It is created by the developer of [TriviaBot](https://triviabot.co.uk) and contributed to by a dedicated team of developers.

*This project is in stable development and accepting PRs and feature requests — Don't be a stranger! If you want to contribute, just get in touch via [GitHub](https://github.com/brainboxdotcc/DPP) or our official [Discord server](https://discord.gg/dpp)!*

<img src="code_editor.png" style="margin-top: 2rem; margin-bottom: 2rem"/><br />

## Downloads

The following downloads are for the most recent version:

* [Source Code](https://github.com/brainboxdotcc/DPP)
* [x64 Linux .deb (64 bit Debian, Ubuntu etc)](https://dl.dpp.dev/latest)
* [x86 Linux .deb (32 bit Debian, Ubuntu etc)](https://dl.dpp.dev/latest/linux-i386)
* [x64 Linux .rpm (64 bit Redhat, CentOS etc)](https://dl.dpp.dev/latest/linux-x64/rpm)
* [x86 Linux .rpm (32 bit Redhat, CentOS etc)](https://dl.dpp.dev/latest/linux-i386/rpm)
* [x64 Windows (64 bit vs2019 release build)](https://dl.dpp.dev/latest/win64-release-vs2019)
* [x64 Windows (64 bit vs2022 release build)](https://dl.dpp.dev/latest/win64-release-vs2022)
* [x64 Windows (64 bit vs2019 debug build)](https://dl.dpp.dev/latest/win64-debug-vs2019)
* [x64 Windows (64 bit vs2022 debug build)](https://dl.dpp.dev/latest/win64-debug-vs2022)
* [ARM6 Linux .deb (32 bit Raspberry Pi 1, 2)](https://dl.dpp.dev/latest/linux-rpi-arm6)
* [ARM7 Linux .deb (32 bit Raspberry Pi 3, 4)](https://dl.dpp.dev/latest/linux-rpi-arm7hf)
* [ARM64 Linux .deb (64 bit Raspberry Pi 4, Smartphones)](https://dl.dpp.dev/latest/linux-rpi-arm64)

You can find further releases in other architectures and formats or the source code on the [GitHub Repository](https://github.com/brainboxdotcc/DPP/releases). For a realtime JSON format list of download all download links, click [here](https://dl.dpp.dev/json)

## Library features

* Support for Discord API v10
* Really small memory footprint
* Efficient caching system for guilds, channels, guild members, roles, users
* Sharding and clustering (Many shards, one process: specify the number of shards, or let the library decide)
* Highly optimised ETF (Erlang Term Format) support for very fast websocket throughput (*no other C++ Discord library has this!*)
* [Slash Commands/Interactions support](https://dpp.dev/slashcommands.html)
* [Voice support](https://dpp.dev/soundboard.html) (sending **and** receiving audio)
* The entire Discord API is available for use in the library
* Stable [Windows support](https://dpp.dev/buildwindows.html)
* Ready-made compiled packages for Windows, Raspberry Pi (ARM64/ARM7/ARMv6), Debian x86/x64 and RPM based distributions
* Highly scalable for large amounts of guilds and users

## Supported Operating Systems

The library runs great on **Linux**. **Windows** is also supported and we offer ready made compiled DLL and LIB files for easy integration into any windows visual studio 2019/2022 project.
**Mac OS X** and **FreeBSD** is also functional and stable, as is running your bot on a **Raspberry Pi** — we offer a prebuilt .deb for ARM64, ARMv6 and ARMv7 to save on having to wait for it to compile.

The library may work fine in other operating systems too, if you run a D++ bot on something not listed here please let us know!

## Getting started
* [GitHub Repository](https://github.com/brainboxdotcc/DPP)
* [Discord Server](https://discord.gg/dpp)
* \subpage frequently-asked-questions "Frequently Asked Questions"
* \ref buildlinux
* \ref buildwindows
* \ref buildosx
* \ref buildfreebsd
* \ref creating-a-bot-application "Creating a Bot Account"
* [Example Programs](/md_docpages_03_example_programs.html)

## Architecture
* \ref clusters-shards-guilds
* \ref thread-model

## Learning Resources
* [C++ for JavaScript Developers](https://pawelgrzybek.com/cpp-for-javascript-developers/)
* [C++ In Four Hours](https://www.youtube.com/watch?v=vLnPwxZdW4Y&vl=en)

