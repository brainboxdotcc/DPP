\page making_threads Creating and Interacting with Threads

A new feature added to Discord recently is `Threads`, these allow you to break off a message into a different "channel", without creating a whole new channel. There are also other types of "thread channels", one example being a `forums channel`. This type of channel only contains threads, meaning you can't send messages in it so if you want to make one of them, be careful about trying to send a message in it!

In this tutorial, we'll be going through:

- How to create a thread.
- How to loop through all the active threads in a server (and sending a message in one).
- How to lock a thread (editing threads).

First, let's go through creating a thread.

\include{cpp} making_threads1.cpp

If all went well, you'll see that the bot has successfully created a thread!

\image html creating_thread.png

Now, let's cover looping through all the threads in a server. For this demonstration, we'll be picking the first thread we find in the list and sending a message in it.

\include{cpp} making_threads2.cpp

After that, you'll be able to see your bot send a message in your thread!

\image html creating_thread_2.png

Those of you who are familar with sending messages in regular channels may have also noticed that sending messages to threads is the same as sending a general message. This is because threads are basically channels with a couple more features!

Now, we're going to cover how to lock a thread! With this, you'll also learn how to edit threads in general, meaning you can go forward and learn how to change even more stuff about threads, as much as your heart desires!

\include{cpp} making_threads3.cpp

Once you've ran that, you'll see that you were successfully able to lock a thread!

\image html creating_thread_3.png