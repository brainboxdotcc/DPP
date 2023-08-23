\page lambdas-and-locals Ownership of local variables and safely transferring into a lambda

If you are reading this page, you have likely been sent here by someone helping you diagnose why your bot is crashing or why seemingly invalid values are being passed into lambdas within your program that uses D++.

It is important to remember that when you put a lambda callback onto a function in D++, that this lambda will execute at some point in the **future**. As with all things in the future and as 80s Sci-Fi movies will tell you, when you reach the future things may well have changed!

\image html delorean-time-travel.gif

To explain this situation and how it causes issues I'd like you to imagine the age old magic trick, where a magician sets a fine table full of cutlery, pots, pans and wine. He indicates to the audience that this is authentic, then with a whip of his wrist, he whips the tablecloth away, leaving the cutlery and other tableware in place (if he is any good as a magician!)

Now imagine the following code scenario. We will describe this code scenario as the magic trick above, in the steps below:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
bot.on_message_create([&bot](const dpp::message_create_t & event) {
	int myvar = 0;
	bot.message_create(dpp::message(event.msg.channel_id, "foobar"), [&](const auto & cc) {
		myvar = 42;
	});
});
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In this scenario, the outer event, `on_message_create` is your tablecloth. The lambda inside the `bot.message_create` is the tableware and cutlery. The following chain of events happens in this code:

* The magician executes his magic trick (D++ the `bot.on_message_create entering` the outer lambda)
* Your code executes `bot.message_create()` inside this outer lambda
* D++ inserts your request to send a message into its queue, in another thread. The inner lambda, where you might later set `myvar = 42` is safely copied into the queue for later calling.
* The tablecloth is whipped away... in other words, `bot.on_message_create` ends, and all local variables including `myvar` become invalid
* At a later time (usually 80ms through to anything up to 4 seconds depending on rate limits!) the message is sent, and your inner lambda which was saved at the earlier step is called.
* Your inner lambda attempts to set `myvar` to 42... but `myvar` no longer exists, as the outer lambda has been destroyed....
* The table wobbles... the cutlery shakes... and...
* Best case scenario: you access invalid RAM no longer owned by your program by trying to write to `myvar`, and [your bot outright crashes horribly](https://www.youtube.com/watch?v=sm8qb2kP-fQ)!
* Worse case scenario: you silently corrupt ram and end up spending days trying to track down a bug that subtly breaks your bot...

The situation I am trying to describe here is one of object and variable ownership. When you call a lambda, **always assume that every non-global reference outside of that lambda will be invalid when the lambda is called**! For any non-global variable always take a **copy** of the variable (not reference, or pointer). Global variables or those declared directly in `main()` are safe to pass as references.

For example, if we were to fix the broken code above, we could rewrite it like this:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
bot.on_message_create([&bot](const dpp::message_create_t & event) {
	int myvar = 0;
	bot.message_create(dpp::message(event.msg.channel_id, "foobar"), [myvar](const auto & cc) {
		myvar = 42;
	});
	std::cout << "here\n";
});
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Note, however that when you set `myvar` within the inner lambda, this does **not effect** the value of the var outside it. Lambdas should be considered self-contained silos, and as they execute in other threads should not be relied upon to set anything that exists **outside of that lambda**.

\warning Always avoid just using `[&]` in a lambda to access all in the scope above. It is unlikely that half of this scope will still even be valid by the time you get a look at it!

Similarly, and important to note, your program **will not wait for bot.message_create to send its message and call its lambda** before continuing on to print `here`. It will instantly insert the request into its queue and bail straight back out (see the steps above) and immediately print the text.

If you do want to get variables out of your lambda, create a class, or call a separate function, and pass what you need into that function from the lambda **by value** or alternatively, you can use `std::bind` to bind a lambda directly to an object's method instead (this is great for modular bots).

If you are stuck, as this is a complex subject please do feel free to ask on the [official support server](https://discord.gg/dpp)!
