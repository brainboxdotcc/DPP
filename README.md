# D++
## An incredibly lightweight C++ Discord library

[![Discord](https://img.shields.io/discord/825407338755653642?style=flat)](https://discord.gg/dpp) 
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/39b054c38bba411d9b25b39524016c9e)](https://www.codacy.com/gh/brainboxdotcc/DPP/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=brainboxdotcc/DPP&amp;utm_campaign=Badge_Grade) 
![Lines of code](https://img.shields.io/tokei/lines/github/brainboxdotcc/DPP) 
[![D++ CI](https://github.com/brainboxdotcc/DPP/actions/workflows/ci.yml/badge.svg)](https://github.com/brainboxdotcc/DPP/actions/workflows/ci.yml)

### Library features:

* Really small memory footprint
* Efficient caching system for guilds, channels, guild members, roles, users
* Sharding (Many shards, one process: specify the number of shards, or let the library decide)
* [Slash Commands/Interactions support](https://dpp.brainbox.cc/slashcommands.html)
* [Voice support](https://dpp.brainbox.cc/soundboard.html)
* The entire Discord API is available for use in the library
* Stable [Windows support](https://dpp.brainbox.cc/buildwindows.html)
* Ready-made compiled packages for Windows, Raspberry Pi (ARM64/ARM7/ARMv6) and Debian x86/x64

Want to help? Drop me a line or send a PR.

This library is in use on [TriviaBot](https://triviabot.co.uk/) and [Sporks bot](https://sporks.gg).

## Documentation

The documentation is a work in progress, generated from the code comments and markdown using Doxygen.

#### [View D++ library documentation](https://dpp.brainbox.cc/)

## Supported Systems

The library runs great on **Linux**. **Windows** is also supported and we offer ready made compiled DLL and LIB files for easy integration into any windows visual studio 2019 project.
**Mac OS X** is also functional and stable, as is running your bot on a **Raspberry Pi** - we offer a prebuilt .deb for ARM64, ARM6 and ARM7 to save on having to wait for it to compile.

The library may work fine in other operating systems too, if you run a D++ bot on something not listed here please let us know!

## 🤝 Contributing

Contributions, issues and feature requests are welcome. After cloning and setting up project locally, you can just submit 
a PR to this repo and it will be deployed once it's accepted.

⚠️ It’s good to have descriptive commit messages, or PR titles so that other contributors can understand about your 
commit or the PR Created. Read [conventional commits](https://www.conventionalcommits.org/en/v1.0.0-beta.3/) before 
making the commit message.

## 💬 Get in touch

If you have various suggestions, questions or want to discuss things with our community, Join our discord server!
Make a humorous reference to brains in your nickname to get access to a secret brain cult channel! :)

[![Discord](https://img.shields.io/discord/825407338755653642?style=flat)](https://discord.gg/dpp)

## Show your support

We love people's support in growing and improving. Be sure to leave a ⭐️ if you like the project and 
also be sure to contribute, if you're interested!

## Dependencies

### External Dependencies (You must install these)
* [cmake](https://cmake.org/) (version 3.13+)
* [g++](https://gcc.gnu.org) (version 8+)
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

