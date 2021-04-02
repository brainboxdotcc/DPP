# D++
## An incredibly lightweight C++ Discord library

This project is in alpha stages of development.

### Completed so far:

* [x] Websocket connection with heartbeat keepalive and connection resuming
* [x] Caching system for guilds, channels, guild members, roles, users
* [x] Event dispatcher - currently only dispatches a subset of messages including e.g. `on_message_create` and `on_guild_create`
* [x] Ability to attach handlers to events
* [x] REST HTTPS call system using cpp-httplib
* [x] Message send (`dpp::cluster::message_create()`)
* [x] Embeds
* [x] Ratelimit system

### To do:

* [ ] Add the rest of the discord events
* [ ] Add the REST of the HTTP calls (pun intended)
* [ ] Ability to receive raw json strings to event handlers
* [ ] Shard manager
* [ ] Cluster management
* [ ] File uploading

Want to help? Drop me a line or send me a PR. I'll be choosy about what PRs i accept whilst the library is in such a heavy state of development.

It is my intention to get this stable enough to use on my production bot, [TriviaBot](https://github.com/brainboxdotcc/triviabot).

## Documentation

Documentation will be here in the project wiki when the project is more complete.

## Supported OSes

At present, **Linux** only. This may change. The Library may build and work on other platforms.

## Dependencies

### External Dependencies (You must install these)
* [cmake](https://cmake.org/) (version 3.13+)
* [g++](https://gcc.gnu.org) (version 8+)
* [OpenSSL](https://openssl.org/) (whichever `-dev` package comes with your OS)

### Included Dependencies (Packaged with the library)
* [nlohmann::json](https://github.com/nlohmann/json)
* [cpp-httplib](https://github.com/yhirose/cpp-httplib)
* [spdlog](https://github.com/gabime/spdlog)

# Setup

## 1. Build Source Code

    mkdir build
    cd build
    cmake ..
    make -j8
    
Replace the number after -j with a number suitable for your setup, usually the same as the number of cores on your machine. `cmake` will fetch any dependencies that are required for you and ensure they are compiled alongside the library.

## 2. Run test cases

run `./test` for unit test cases.

## 3. Install to /usr/local/include and /usr/local/lib

`make install` coming soon!
