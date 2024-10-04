\page using-emojis Using Emojis

Need your bot to use an emoji? Then you've come to the right place! Here are three examples of using emojis.

\note If your bot isn't in the guild you want to use the custom emoji from, it won't work, giving you dpp::err_unknown_emoji.

First - Sending emojis. You have to use its mention, which depends on the type. If it's a default emoji, you use the corresponding character. So, for example, if you wanted to send a nerd emoji, you would use the nerd unicode character. Now, custom emoji. There are two types: static and animated. Their mentions are `<:[name]:[id]>` and `<a:[name]:[id]>`, where `[name]` means the emoji name and `[id]` is for its ID. When you send such mention, it automatically gets converted into your emoji. Here's an example of sending emojis:

\include{cpp} using_emojis1.cpp

Now, our bot will send our epic emojis!

\image html using_emojis1.png

Second - Reacting to messages. Sometimes there's something so interesting in the chat that we want to react to it. While we see the emoji we react with, for bots, it's some plain text. There are different formats for different kinds of emoji when reacting too. For unicode, it's simply its character, like when sending. For custom ones it's either `[name]:[id]` (if static) or `a:[name]:[id]` (if animated). Let's show our bot's honest reactions!

\include{cpp} using_emojis2.cpp

Yay, our bot has emotions now!

\image html using_emojis2.png

Finally, select menus. These guys are covered \ref components3 "here". They require emoji components (name, ID, animated state) to come separately. If the emoji you're using isn't animated, you don't have to specify that. If your emoji is unicode, it doesn't even have an ID, so you only put the character, since both animated state and ID are defaulted to none (false/0).

\include{cpp} using_emojis3.cpp

Yay, our context menu is now interesting!

\image html using_emojis3.png
