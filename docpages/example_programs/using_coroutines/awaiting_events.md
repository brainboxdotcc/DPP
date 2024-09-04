\page awaiting-events Waiting for events

\include{doc} coro_warn.dox

D++ makes it possible to await events: simple use `co_await` on any of the event routers, such as \ref dpp::cluster::on_message_create "on_message_create", and your coroutine will be suspended until the next event fired by this event router. You can also `co_await` the return of an event router's \ref dpp::event_router_t::when "when()" method while passing it a predicate function object, it will only resume your coroutine when the predicate returns true. Be aware that your coroutine is attached to the event router only when you call `co_await` and not before, and will be detached as it is resumed.

\note When the event router resumes your coroutine, it will give you __a reference to the event object__. This will likely mean it will be destroyed after your next co_await, make sure to save it in a local variable if you need it for longer.

\include{cpp} coro_awaiting_events.cpp

Note that there is a problem with that! If the user never clicks your button, or if the message gets deleted, your coroutine will be stuck waiting... And waiting... Forever until your bot shuts down, occupying a space in memory. This is where the \ref expiring-buttons "next example" comes into play as a solution, with a button that expires with time.

\image html waiting_coroutine.jpg
