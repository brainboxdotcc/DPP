# D++
## An incredibly lightweight C++ Discord library

[![Discord](https://img.shields.io/discord/825407338755653642?style=flat)](https://discord.gg/dpp) 
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/39b054c38bba411d9b25b39524016c9e)](https://www.codacy.com/gh/brainboxdotcc/DPP/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=brainboxdotcc/DPP&amp;utm_campaign=Badge_Grade) 
![Lines of code](https://img.shields.io/tokei/lines/github/brainboxdotcc/DPP) 
[![D++ CI](https://github.com/brainboxdotcc/DPP/actions/workflows/ci.yml/badge.svg)](https://github.com/brainboxdotcc/DPP/actions/workflows/ci.yml)

### Library features:

* Really small memory footprint
* Efficient caching system for guilds, channels, guild members, roles, users
* Sharding and clustering (Many shards, one process: specify the number of shards, or let the library decide)
* Highly optimised ETF (Erlang Term Format) support for very fast websocket throughput (*no other C++ Discord library has this!*)
* [Slash Commands/Interactions support](https://dpp.dev/slashcommands.html)
* [Voice support](https://dpp.dev/soundboard.html) (sending **and** receiving audio)
* The entire Discord API is available for use in the library
* Stable [Windows support](https://dpp.dev/buildwindows.html)
* Ready-made compiled packages for Windows, Raspberry Pi (ARM64/ARM7/ARMv6), Debian x86/x64 and RPM based distributions

Want to help? Drop me a line or send a PR.

This library is in use on [TriviaBot](https://triviabot.co.uk/) and [Sporks bot](https://sporks.gg) and many other bots!

## Documentation

The documentation is a work in progress, generated from the code comments and markdown using Doxygen.

#### [View D++ library documentation](https://dpp.dev/)

## Supported Systems

The library runs great on **Linux**. **Windows** is also supported and we offer ready made compiled DLL and LIB files for easy integration into any windows visual studio 2019 or 2022 project.
**Mac OS X** and **FreeBSD** is also functional and stable, as is running your bot on a **Raspberry Pi** - we offer a prebuilt .deb for ARM64, ARM6 and ARM7 to save on having to wait for it to compile.

If you are on windows, and just want to get started as quickly as possible, you should look at our [Windows Bot Template repository](https://github.com/brainboxdotcc/windows-bot-template). This repository can be cloned and will get you up and running in a matter of minutes, with a pre-built D++ in a ready to go project for Visual Studio 2019 and 2022.

The library may work fine in other operating systems too, if you run a D++ bot on something not listed here please let us know!

## 🤝 Contributing

Contributions, issues and feature requests are welcome. After cloning and setting up project locally, you can just submit 
a PR to this repo and it will be deployed once it's accepted.

Please read the [D++ Code Style Guide](https://dpp.dev/coding-standards.html) for more information on how we format pull requests.

## 💬 Get in touch

If you have various suggestions, questions or want to discuss things with our community, Join our discord server!
Make a humorous reference to brains in your nickname to get access to a secret brain cult channel! :)

[![Discord](https://img.shields.io/discord/825407338755653642?style=flat)](https://discord.gg/dpp)

## Show your support

We love people's support in growing and improving. Be sure to leave a ⭐️ if you like the project and also be sure to contribute, if you're interested!

## Dependencies

### Build requirements
* [cmake](https://cmake.org/) (version 3.13+)
* A supported C++ compiler from the list below

### Supported compilers
* [g++](https://gcc.gnu.org) (version 8 or higher)
* [clang](https://clang.llvm.org/)
* AppleClang (12.0 or higher)
* Microsoft Visual Studio 2019 or 2022 (16.x/17.x)
* [mingw-w64](https://www.mingw-w64.org/) (gcc version 8 or higher)
Other compilers may work (either newer versions of those listed above, or different compilers entirely) but have not been tested by us.

### External Dependencies (You must install these)
* [OpenSSL](https://openssl.org/) (whichever `-dev` package comes with your OS)
* [zlib](https://zlib.net) (whichever `-dev` package comes with your OS)

#### Optional Dependencies
For voice support you require both of:
* [LibOpus](https://www.opus-codec.org)
* [libsodium](https://github.com/jedisct1/libsodium)

### Included Dependencies (Packaged with the library)
* [nlohmann::json](https://github.com/nlohmann/json)
* [fmt::format](https://github.com/fmt/format)
* [cpp-httplib](https://github.com/yhirose/cpp-httplib)

