\page using-emoji Using Emoji

Need your bot to use an emoji? Then the `dpp::emoji` class is just for you. It can store either a custom or a unicode emoji's name and ID. To use one in a message or when reacting to one, you use its mention. In select menus, it's the emoji object itself. Here's how to use them:

\include{cpp} using_emoji.cpp

Now, our bot will react to our messages!

\image html using_emoji.png