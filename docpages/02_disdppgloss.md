\page glossary-of-common-discord-terms A Glossary of Common Discord Terms

[TOC]

This is a list of terms that you should know if you want to use D++ (or any other Discord library). These terms are not D++ specific, and are commonly used throughout most of the Discord developer community. This list, with a few exceptions, ***is Discord specific***, that is to say that this is not an explanation of commonly used C++ terms, it is for people who are not familiar with the terminology of the Discord API and libraries themselves.

## Glossary

Listed in alphabetical order, with terms in bold, here are the basics on...

* **Action row**: A collection of up to five **components** which is attached to a message.

* **Application Command**: See \ref slashcommand.

* **Audit log**: A log of **events** that have happened in a **guild**.

* **Auto mod**: Discord's low-code solution to moderation. However, it is very limited in scope.

* **Badge**: A decoration on someone's profile showing certain things about them, such as if they have Nitro, if they are a Discord developer, etc.

* **Bot token**: A secret string of characters that is used as a "login" to your bot. If you lose it or it gets leaked you will have to get a new one from the Discord developer portal, so be sure to keep it in a place that is both secure and where you won't forget it.

* **Button**: A **component** on a message that can be styled that sends an **event** when clicked on by a user.

* **Cache**: A type of storage efficient for things like messages.

* **Callback**: While not strictly related to Discord, it is used a LOT in D++. A callback is when a function is passed to another function, sort of like how you might give someone a telephone number (you give them the means to do some sort of interaction rather than asking them how to interact), which is used to handle responses to **events**.

* **Cluster**: A singular bot application, which is composed of one or more **shards**, a **cluster** is the centre of bot development.

* **Component**: A component is anything that can appear in a bot's message besides text, such as **buttons** and **select menus**.

* **Drop down/Select menu**: A **component** of a message that upon being clicked drops down and allows the user to select an option.

* **Embeds**: A widget attached to a message which can contain multiple fields of texts, an image, and much more information.

* **Ephemeral**: A message only visible to the user being replied to.

* **Event**: Something that a Discord bot can respond to, such as a message being sent, a **button** being clicked, or an option being selected, among others.

* **Guild**: What the Discord API (and most libraries for it) call a server.

* **Intents**: The right for a bot to receive certain data from the Discord API.

* **Interaction**: An object that contains information about whenever a user interacts with an application, such as sending a message or clicking a button. It is the main part of an **event** that will be accessed in an application.

* **Modal**: A pop up form that contains text that can be sent by a bot.

* **[Shards](\ref clusters-shards-guilds)**: A shard manages part of the workload of your Discord application.

* **Slashcommand**: The primary way a user interacts with a bot. It is a command sent to the bot with **parameters** (which may be optional) and is initiated by beginning a message with `/`. \anchor slashcommand

* **Snowflake**: An unsigned 64 bit integer (it can represent anything from 0 to 2^64-1) that is used by Discord to identify basically everything, including but not limited to, **guilds**, users, messages, and much more.

* **Subcommands**: A command which is derived from a different command, such as a bot that allows a person to get statistics for Discord might have a `stats guild` command and a `stats global` command, both of which are **subcommands** of `stats`.