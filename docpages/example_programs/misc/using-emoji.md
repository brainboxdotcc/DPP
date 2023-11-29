\page using-emoji Using Emoji

Need your bot to use an emoji?
Then the `dpp::emoji` class is just for you. It can store either a custom or a unicode emoji's name and ID. To use one in a message, you use its mention. To react, there's other kind of format: if you want to use an animated one, it's a:<name>:id, if it's static — <name>:id, if that's a unicode one — the corresponding character. Here, <name> represents the name of the emoji. In select menus, it's the emoji object itself. Here's how to use them: 

\include{cpp} using_emoji.cpp

Now, our bot will react to our messages!

\image html using_emoji.png