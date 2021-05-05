# Welcome to the D++ developer wiki!

## What is D++ (DPP)?

<img src="DPP-Logo.png" align="right" style="max-width: 20% !important"/>
D++ is a lightweight and simple library for Discord written in modern C++. It is designed to cover as much of the API specification as possible and take very little 
memory to do so, even when caching large amounts of the data to cut down on HTTP requests.

It is created by the developer of [TriviaBot](https://triviabot.co.uk) and contributed to by a dedicated team of developers.

This project is in beta stages of development.

## Library features:

* Really small memory footprint
* Efficient caching system for guilds, channels, guild members, roles, users
* Sharding (Many shards, one process: specify the number of shards, or let the library decide)
* Voice support
* Slash command/interaction support
* Pretty much the entire API is supported
* Windows support

Want to help? Drop me a line or send me a PR. I'll be choosy about what PRs i accept whilst the library is in such a heavy state of development.

## Supported Operating Systems

The library runs best on Linux. Windows is supported via cmake and Visual Studio 2019 but not encouraged for production use. The library may work fine in other operating systems too, but with no access to these we cannot support them.

## Getting started
* [GitHub Repository](https://github.com/brainboxdotcc/DPP)
* [Discord Server](https://discord.gg/dpp)
* [Frequently Asked Questions](docpages/Z_01_FAQ.md)
* [Building on Linux](docpages/Z_02_BUILD_LINUX.md)
* [Building on Windows](docpages/Z_03_BUILD_WIN.md)
* [Example Programs](docpages/Z_04_EXAMPLE.md)

## Reference
* Library Types, Classes and Structs
* [REST Calls](docpages/Z_07_REST.md)
* [Event Handlers](docpages/Z_06_EVENT.md)

## Architecture
* [Clusters, Shards and Guilds](docpages/Z_05_CLUSTERS.md)
* [Thread model](docpages/Z_06_THREADMODEL.md)

## Learning Resources
* [C++ for JavaScript Developers](https://pawelgrzybek.com/cpp-for-javascript-developers/)
* [C++ In Four Hours](https://www.youtube.com/watch?v=vLnPwxZdW4Y&vl=en)
