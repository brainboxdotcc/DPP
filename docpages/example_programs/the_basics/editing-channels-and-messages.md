\page editing-channels-and-messages Editing Channels and Messages

Sometimes we need to update an object, such as a message (whether it's plain text or an embed) or a channel. At first, it might seem confusing, but it's actually really simple! You need an object with all of the properties being identical to the existing one. Say you're editing a message. You need to have an object identical to the one in Discord. Then you replace what you need such as its content. NOTE: your bot can't edit messages sent by others.

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
