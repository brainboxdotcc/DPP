\page detecting-messages Listening to messages

Sometimes, you may want to listen out for a message, rather than a command. This could be used for many cases like a spam filter, a bot that would respond to movie quotes with gifs, or a chat bot! However, in this page, we'll be using this to create a moderation filter (detect bad words).

\warning As of August 30th, 2022, Discord made Message Content a privileged intent. Whilst this means you can still use prefixed messages as commands, Discord does not encourage this and heavily suggests you use \ref slashcommands "slash commands". If you wish to create commands, use \ref slashcommands "slash commands", not messages.

\include{cpp} detecting_messages.cpp

If all went well, you should have something like this!

\image html badwordfilter.png