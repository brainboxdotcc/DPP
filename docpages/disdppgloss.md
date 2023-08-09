## A Glossary of Common Discord Terms

Here is a list of common terms that are both terms one should know if one is to use D++ (or any other discord library) however are terms that are not commonly used throughout the discord community. This list, with a few exceptions, ***is discord specific***, that is to say, this is not a explanation of commonly used C++ terms, it is for people who are not familiar with the terminology of the discord API.

#### Glossary

Listed in alphabetical order, with terms in bold, here are the basics on...

1. **Action row**: A collection of up to five **components**, which is attached to a message

2. **Audit log**: A log of **events**, that have happened in a **guild**

3. **Auto mod**: Discord's low-code solution to moderation, however it is very limited in scope

4. **Badge**: A decoration on someone's profile showing certain things about them, such as if they have nitro, if they are a discord developer, etc.

5. **Bot token**: A secret string of characters, this is how you login to your bot, if you lose it or it gets leaked, you will have to get a new one from the discord developer portal, and it is necessary to use in your bot

6. **Button**: A **component** on a message that can be styled, that sends an **event** when clicked on by a user

7. **Cache**: A type of storage efficient for things like messages

8. **Callback**: While not strictly related to discord, it is used a LOT in D++, so a callback is when a function is passed to another function, sort of like how you might give someone a telephone number (you give them the means to do some sort of interaction rather than asking them how to interact), which is used to handle responses to **events**

9. **Cluster**: A singular bot application, which is composed of one or more **shards**, a **cluster** is the center of bot development 

10. **(Slash) command**: The primary way a user interacts with a bot, it is a command sent to the bot with **parameters**, which may be optional, and is initiated by staring a message with `/`.

11. **Component**: A component is anything that can appear in a bot's message besides text, such as **buttons** and **select menus**.

12. **Drop down/Select menu**: A **component** of a message that upon being clicked, drops down and allows the user to select an option

13. **Embeds**: A widget attached to a message, which can contain multiple fields of texts, an image, and much more information 

14. **Ephemeral**: A message only visible to the user being replied to

15. **Event**: Something that a Discord bot can respond to, such as a message being sent, a **button** being clicked, or an option being selected, among others.

16. **Guild**: What the Discord API (and most libraries for it) call a server

17. **Intents**: The right for a bot to receive certain data from the Discord API 

18. **Interaction**: Something that can be responded to, and it is the main part of an **event** that will be accessed in an application 

19. **Modal**: A pop up form that contains text that can be sent by a bot

20. [**Shards and clusters**](\ref clusters-shards-guilds): A cluster is the main container for an application, which contains many shards, each of which manage a subset of your workload, fortunately, D++ does this automatically 

21. **Snowflake**: An unsigned 64 bit integer (it can represent anything from 0 to 2^64) that is used by discord to identify basically everything, including but not limited to, **guilds**, users, messages, and much more

22. **Subcommands**: A command which is derived from a different command, for example a bot that allows a person to get statistics for discord might have a `stats guild` command and a `stats global` command both of which are **subcommands** of `stats`
