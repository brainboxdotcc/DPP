\page coding-standards Coding Style Standards

This page lists the coding style we stick to when maintaining the D++ library. If you are submitting a pull request or other code contribution to the library, you should stick to the styles listed below. If something is not covered here, ask on the [official Discord server](https://discord.gg/dpp)!

## Class Names, Function Names, and Method Names

All class, variable/member, function and method names should use `snake_case`, similar to the style of the C++ standard library.

## Enums

Enums and their values should be `snake_case` as with class, function and method names. You do not need to use `enum class`, so make sure that enum values are prefixed with a prefix to make them unique and grouped within the IDE, e.g. `ll_debug`, `ll_trace`, etc.

## Curly Braces, Brackets, etc.

This covers your standard Curly Braces (commonly known as squiggly brackets), and lists.

### Curly Braces

Curly Braces should be on the same line as the keyword, for example:

\include{cpp} coding_style_standards/curly_braces.cpp

This applies to functions, `while` statements, `if` statements, lambdas, nearly anything that uses curly braces with statements!

### Lists

Lists should have a space after the comma in parameter lists, and after opening brackets and before closing brackets except when calling a function, for example:

\include{cpp} coding_style_standards/lists.cpp

## Dot (.) Notation
When using the dot notation repeatedly (For example, creating an embed.), you should start each `.function()` on a new line, as such:

\include{cpp} coding_style_standards/dot_notation.cpp

## Indentation

Indentation should always be tab characters. It is up to you how wide you set tab characters in your editor for your personal tastes. All code blocks delimited within curly braces should be indented neatly and uniformly.

## Constants and \#define Macros

Constants and macros should be all `UPPERCASE` with `SNAKE_CASE` to separate words. Macros should not have any unexpected side effects.

## Comments

All comments should be in `doxygen` format (similar to javadoc). Please see existing class definitions for an example. You should use doxygen style comments in a class definition inside a header file. Be liberal with comments, especially if your code makes any assumptions! Comments should follow the format below:

\note Comments that contain doxygen stuff need to use two stars at the beginning (/**). This example doesn't because doxygen gets confused and doesn't show the comments.

\include{cpp} coding_style_standards/comments.cpp

## Spell Checks

To prevent typos, a GitHub-Action checks the documentation. If it fails for a word that was falsely flagged, you can add them to `.cspell.json`.

## Symbol Exporting

If you export a class which is to be accessible to users, be sure to prefix it with the `DPP_EXPORT` macro, for example:

\include{cpp} coding_style_standards/symbol_exporting.cpp

The `DPP_EXPORT` macro ensures that on certain platforms (notably Windows) the symbol is exported to be available to the library user.

## Public vs Private vs Protected

It is a design philosophy of D++ that everything possible in a class should be public, unless the user really does not need it (you should consider justifying in comments why) or user adjustment of the variable could badly break the functioning of the library. Avoid the use of accessors for setting/getting values in a class, except for bit fields, where you should provide accessors for setting and getting individual bits (for example, see `user.h`), or in the event you want to provide a "fluent" interface. The exception to this is where you want to provide a logic validation of a field, for example if you have a string field with a minimum and maximum length, you can provide a setter the user can *optionally use* which will validate their input.

## Exceptions

All exceptions thrown should derive from dpp::exception (see `dpp/exception.h`) - when validating string lengths, a string which is too long should be truncated using dpp::utility::utf8substr and any strings that are too short should throw a dpp::length_exception.

## Inheritance

Keep levels of inheritance low. If you need to inherit more than 3 levels deep, it is probable that the design could be simplified. Remember that at scale, there can be tens of millions of certain classes and each level of virtual nesting adds to the `vtable` of that object's instance in RAM.

## Bit Field Packing

Where Discord provides boolean flags, if the user is expected to store many of the objects in RAM, or in cache, you should pack all these booleans into bit fields (see `user.h` and `channel.h` for examples). In the event that the object is transient, such as an interaction or a message, packing the data into bit fields is counter intuitive. Remember that you should provide specific accessors for bit field values!

## Keep Dependencies Internal!

Where you are making use of an external dependency such as `opus` or `libssl`, do not place references to the types/structs, or the header files of these external libraries within the header files of D++. Doing so adds that library as a public dependency to the project (which is bad!). Instead make an opaque class, and/or forward-declare the structs (for examples see `sslclient.h` and `discordvoiceclient.h`).

## API Type Names

Where Discord provides a name in PascalCase we should stick as closely to that name as possible but convert it to `snake_case`. For example, GuildMember would become `guild_member`.

## Don't Introduce Any Platform-Specific Code

Do not introduce platform specific (e.g. Windows only) code or libc functions. If you really must use these functions safely wrap them e.g. in `#ifdef _WIN32` and provide a cross-platform alternative so that it works for everyone.

## C++ Version

The code must work with the C++17 standard, unless, for an optional feature that can be enabled and that uses a more recent standard (e.g. Coroutines).

## Select the Right Size Type for Numeric Types

If a value will only hold values up to 255, use `uint8_t`. If a value cannot hold over 65536, use `uint16_t`. These types can help use a lot less RAM at scale.

## Fluent Design

Where possible, if you are adding methods to a class you should consider fluent design. Fluent design is the use of class methods that return a reference to self (via `return *this`), so that you can chain object method calls together (in the way dpp::message and dpp::embed do). For example:

\include{cpp} coding_style_standards/fluent_design.cpp

This would allow the user to do this:

\include{cpp} coding_style_standards/fluent_design2.cpp

## Keep All D++ Related Types in the dpp Namespace

All types for the library should be within the `dpp` namespace. There are a couple of additional namespaces, e.g. dpp::utility for static standalone helper functions and helper classes, and dpp::events for internal websocket event handlers.

## Commit Messages and Git

All pull requests ("PRs") should be submitted against the `dev` branch in GitHub.

### Naming Conventions

It’s good to have descriptive commit messages, or PR titles so that other contributors can understand about your commit or the PR Created. Commits must be prefixed with a type, which consists of a noun, `feat`, `fix`, etc., followed by a colon and a space. Other commit types can be `breaking`, `docs`, `refactor`, `deprecate`, `perf`, `test`, `chore`, and `misc`. Read [conventional commits](https://www.conventionalcommits.org/en/v1.0.0-beta.3/) for more information on how we like to format commit messages.

### GitHub Actions

All PRs must pass the [GitHub Actions](https://github.com/brainboxdotcc/DPP/actions) tests before being allowed to be merged. This is to ensure that no code committed into the project fails to compile on any of our officially supported platforms or architectures.

### Developer Certificate of Origin

All code contributed to D++ must be submitted under agreement of the Linux Foundation Developer Certificate of Origin. This is a simple agreement which protects you and us from any potential legal issues:

```
Version 1.1

Copyright (C) 2004, 2006 The Linux Foundation and its contributors.

Everyone is permitted to copy and distribute verbatim copies of this
license document, but changing it is not allowed.

By making a contribution to this project, I certify that:

(a) The contribution was created in whole or in part by me and I
    have the right to submit it under the open source license
    indicated in the file; or

(b) The contribution is based upon previous work that, to the best
    of my knowledge, is covered under an appropriate open source
    license and I have the right under that license to submit that
    work with modifications, whether created in whole or in part
    by me, under the same open source license (unless I am
    permitted to submit under a different license), as indicated
    in the file; or

(c) The contribution was provided directly to me by some other
    person who certified (a), (b) or (c) and I have not modified
    it.

(d) I understand and agree that this project and the contribution
    are public and that a record of the contribution (including all
    personal information I submit with it, including my sign-off) is
    maintained indefinitely and may be redistributed consistent with
    this project or the open source license(s) involved.
```
