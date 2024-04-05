\page using_timers Using Timers

Timers are a great way to run something every x seconds, from setting the bot's status, to maybe even doing a http request! Luckily, D++ makes this incredibly easy by providing an easy-to-use timer system! This tutorial will show you a couple examples on how to use timers!

First, we'll cover sending the D++ logo every 10 seconds!

\include{cpp} timers_example1.cpp

If all went well, you should get the D++ logo sent every 10 seconds to your desired channel!

\image html timers_example1.png

Now, let's make the same timer a one-shot timer, meaning it will only run once!

\include{cpp} timers_example2.cpp

Great! Now we've learnt the basics of timers and how to stop them!

To finish off, let's make a timer that you can start and stop with commands. This example will store the timer in a map where the user is the owner of the timer!

\include{cpp} timers_example3.cpp

If that went well, it should work something like below!

\image html timers_example3.png

Great, now you've learnt how to store timers to manage at a later point!