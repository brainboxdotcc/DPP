\page making_threads Creating and talking in a thread

A new feature added to Discord recently is `Threads`, these allow you to break off a message into a different "channel", without creating a whole new channel. There are also other types of "thread channels", one example being a `forums channel`. This type of channel only contains threads, meaning you can't send messages in it so if you want to make one of them, be careful about trying to send a message in it!

In this tutorial, we'll be going through how to create a thread and how to talk in a thread.

First, let's go through creating a thread.

\include{cpp} making_threads1.cpp

If all went well, you'll see that the bot has successfully created a thread!

\image html creating_thread.png

Now, let's cover talking in that thread from a channel. It's worth noting that we will be assuming that the thread you just created is the only thread in your server!

\include{cpp} making_threads2.cpp

After that, you'll be able to see your bot send a message in your thread!

\image html creating_thread_2.png