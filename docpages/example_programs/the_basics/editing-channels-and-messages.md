\page editing-channels-and-messages Editing Channels and Messages

Sometimes we need to update an object, such as a message (whether it's plain text or an embed) or a channel. At first, it might seem confusing, but it's actually really simple! You just need to use an object with identical properties you don't need to update. NOTE: your bot can't edit messages sent by others.

\note This example uses callback functions and embeds. To see more information about them, visit \ref callback-functions and \ref embed-message.

\include{cpp} editing_messages.cpp

Before editing:

\image html stuff_edit1.png

After editing:

\image html stuff_edit2.png

Now let's send the embed:

\image html stuff_edit3.png

And finally, edit it:

\image html stuff_edit4.png
