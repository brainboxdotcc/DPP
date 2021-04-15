# D++
## An incredibly lightweight C++ Discord library

[![Codacy Badge](https://app.codacy.com/project/badge/Grade/39b054c38bba411d9b25b39524016c9e)](https://www.codacy.com/gh/brainboxdotcc/DPP/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=brainboxdotcc/DPP&amp;utm_campaign=Badge_Grade) [![CircleCI](https://circleci.com/gh/brainboxdotcc/DPP.svg?style=svg)](https://circleci.com/gh/brainboxdotcc/DPP)


This project is in alpha stages of development.

### Library features:

* Really small memory footprint
* Efficient caching system for guilds, channels, guild members, roles, users
* Sharding (Many shards, one process: specify the number of shards, or let the library decide)
* Voice support
* Pretty much the entire API is supported except for slash commands
* [Windows support](https://dpp.brainbox.cc/a00006.html)

Want to help? Drop me a line or send me a PR. I'll be choosy about what PRs i accept whilst the library is in such a heavy state of development.

It is my intention to get this stable enough to use on my production bot, [TriviaBot](https://github.com/brainboxdotcc/triviabot).

## Documentation

Documentation is a work in progress. Want to contribute? Fork the wiki and submit a PR!

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

