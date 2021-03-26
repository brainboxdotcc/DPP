# D++
## An incredibly lightweight C++ discord library

This project is in alpha stages of development.

Completed so far:

* Websocket connection with heartbeat keepalive and connection resuming

To do:

* Add the rest of the discord events
* Ability to attach to events, and either receive parsed objects or raw json strings
* REST HTTPS call system using cpp-httplib
* Message send/edit/delete
* Embeds
* Ratelimit system
* Shard manager
* Cluster management
* File uploading

(Basically everything!)

Want to help? Drop me a line or send me a PR. I'll be choosy about what PRs i accept whilst the library is in such a heavy state of development.

It is my intention to get this stable enough to use on my production bot, [TriviaBot](https://github.com/brainboxdotcc/triviabot).

## Documentation

Documentation will be here in the project wiki when the project is more complete.

## Supported OSes

At present, **Linux** only. This may change.

## Dependencies

* [cmake](https://cmake.org/) (version 3.13+)
* [g++](https://gcc.gnu.org) (version 8+)
* gdb for debugging on development servers (any recent version)
* [nlohmann::json](https://github.com/nlohmann/json)
* [OpenSSL](https://openssl.org/) (whichever -dev package comes with your OS)

# Setup

## 1. Build Source Code

    mkdir build
    cd build
    cmake ..
    make -j8
    
Replace the number after -j with a number suitable for your setup, usually the same as the number of cores on your machine.

## 2. Run test cases

run `./test` for unit test cases

## 3. Install to /usr/local/include and /usr/local/lib

`make install` coming soon!
