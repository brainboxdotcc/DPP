# D++
## An incredibly lightweight C++ Discord library

[![Discord](https://img.shields.io/discord/825407338755653642?style=flat)](https://discord.gg/RnG32Ctyq7) [![Codacy Badge](https://app.codacy.com/project/badge/Grade/39b054c38bba411d9b25b39524016c9e)](https://www.codacy.com/gh/brainboxdotcc/DPP/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=brainboxdotcc/DPP&amp;utm_campaign=Badge_Grade) ![Lines of code](https://img.shields.io/tokei/lines/github/brainboxdotcc/DPP) [![CircleCI](https://circleci.com/gh/brainboxdotcc/DPP.svg?style=svg)](https://circleci.com/gh/brainboxdotcc/DPP) [![GitHub Actions](https://img.shields.io/endpoint.svg?url=https%3A%2F%2Factions-badge.atrox.dev%2Fbrainboxdotcc%2FDPP%2Fbadge&label=build&logo=none)](https://actions-badge.atrox.dev/brainboxdotcc/DPP/goto)

This project is in late alpha stages of development.

### Library features:

* Really small memory footprint
* Efficient caching system for guilds, channels, guild members, roles, users
* Sharding (Many shards, one process: specify the number of shards, or let the library decide)
* [Slash Commands/Interactions suppport](https://dpp.brainbox.cc/a00014.html)
* [Voice support](https://dpp.brainbox.cc/a00012.html)
* Pretty much the entire Discord API is available for use in the library
* [Windows support](https://dpp.brainbox.cc/a00006.html)

Want to help? Drop me a line or send a PR.

This library is in use on [TriviaBot](https://triviabot.co.uk/) and [Sporks bot](https://sporks.gg).

## Documentation

The documentation is a work in progress, generated from the code comments and markdown using Doxygen.

#### [View D++ library documentation](https://dpp.brainbox.cc/)

Do you want to contribute? Fork the code and submit a PR!

## Supported OSes

The library runs best on **Linux**. **Windows** is supported via cmake and Visual Studio 2019 but not encouraged for production use.
The library may work fine in other operating systems too, but with no access to these we cannot support them.

## Dependencies

### External Dependencies (You must install these)
* [cmake](https://cmake.org/) (version 3.13+)
* [g++](https://gcc.gnu.org) (version 8+)
* [OpenSSL](https://openssl.org/) (whichever `-dev` package comes with your OS)

#### Optional Dependencies
* [LibOpus](https://www.opus-codec.org) and [libsodium](https://github.com/jedisct1/libsodium) for voice support

### Included Dependencies (Packaged with the library)
* [nlohmann::json](https://github.com/nlohmann/json)
* [fmt::format](https://github.com/fmt/format)
* [cpp-httplib](https://github.com/yhirose/cpp-httplib)

