\page using-emoji Sending Emoji

Need your bot to use emoji? Then you've come to the right place! Here are shown 3 cases of where and how you would use emoji.

First - sending emoji. You have to use its mention, which depends on the type. If it's a default emoji, you use the corresponding character. So, for example, if you wanted to send a nerd emoji, you would use the nerd unicode character. Now, custom emoji. There are two types: static and animated. Their mentions are `<:[name]:[id]>` and `<a:[name]:[id]>` consequently, where `[name]` means the emoji name and `[id]` is for its ID. When you send such mention, it automatically gets converted into your emoji. Here's an example of sending emojis:

\include{cpp} using_emoji1.cpp

Now, our bot will send our epic emojis!

\image html using_emoji1.png

Sometimes there's something so interesting in the chat that we want to react to it. And while we see the emoji we react with, for bots, it's some plain text. There are different formats for different kinds of emoji when reacting, too. For unicode, it's simply its character, like when sending. For custom ones it's either `[name]:[id]` (if static) or `a:[name]:[id]` (if animated). Let's show our bot's honest reactions!

\include{cpp} using_emoji2.cpp

Yay, our bot has emotions now!

\image html using_emoji2.png

Finally, select menus. They're lists that let you select an option with an emoji each and are an optional part of a message. These guys are covered \ref components3 "here". They require emoji components (name, ID, animated state) to come separately. Also, if the emoji you're using isn't animated, you don't have to specify that. If your emoji is unicode, the ID is optional, too, since both are defaulted to none (0/false).

\include{cpp} using_emoji3.cpp

The Cook appears.

\image html using_emoji3.png