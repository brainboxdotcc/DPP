\page editing-channels-and-messages Editing Channels and Messages

Sometimes we need to update an object, such as a message (whether it's plain text or an embed) or a channel. At first, it might seem confusing, but it's actually really simple! You need an object with all the properties being identical to the existing one. Say you're editing a message. You need to have an object with its ID the same as the one in Discord. Then you replace what you need, such as its content.

\note This example uses callback functions and embeds. To see more information about them, visit \ref callback-functions and \ref embed-message.

## Editing messages
Here we send a message and edit it after. To do so, we first reply to the command `msg-send` with some text, "This is a message" in our case. As described above, on the next step the message object is taken and the text is replaced with whatever the user desires.

\include{cpp} editing_messages1.cpp

\note Your bot can't edit messages sent by others!\ref embed-message.

Before editing the message:

\image html stuff_edit1.png

After editing the message:

\image html stuff_edit2.png

## Editing channels
Now we'll want to edit an existing channel - its name in this case. This works similarly to how messages are edited.

\include{cpp} editing_messages2.cpp

Before editing the channel:

\image html stuff_edit3.png

After editing the channel:

\image html stuff_edit4.png

## Editing embeds
Now let's send an embed and edit it. If a message has one `content` field, it can have a few `embed` fields, up to 10 to be precise. So we first get the embed we want and edit and change its description.

\include{cpp} editing_messages3.cpp

Before editing the embed:

\image html stuff_edit5.png

Finally, after editing the embed:

\image html stuff_edit6.png
