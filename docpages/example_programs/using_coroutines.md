\page using-coroutines Using Coroutines

\include{doc} coro_warn.dox

One of the most anticipated features of C++20 is the addition of coroutines : in short, they are functions that can be paused while waiting for data and resumed when that data is ready. They make asynchronous programs much easier to write, but they do come with additional dangers and subtleties.

* \subpage coro-introduction "Introduction to coroutines"
* \subpage coro-simple-commands "Simple coroutine commands"
* \subpage awaiting-events "Waiting for events"
* \subpage expiring-buttons "Making expiring buttons with when_any"
