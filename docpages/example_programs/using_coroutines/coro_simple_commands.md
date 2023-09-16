\page coro-simple-commands Making simple commands

\include{doc} coro_warn.dox

### Several steps in one

\note The next example assumes you are already familiar with how to use \ref firstbot "slash commands", \ref slashcommands "parameters", and \ref discord-application-command-file-upload "sending files through a command".

With coroutines, it becomes a lot easier to do several asynchronous requests for one task. As an example an "addemoji" command taking a file and a name as a parameter. This means downloading the emoji, submitting it to Discord, and finally replying, with some error handling along the way. Normally we would have to use callbacks and some sort of object keeping track of our state, but with coroutines, the function can simply pause and be resumed when we receive the response to our request :

\include{cpp} coro_simple_commands1.cpp

### I heard you liked tasks

\note This next example is fairly advanced and makes use of many of both C++ and D++'s advanced features.

Earlier we mentioned two other types of coroutines provided by dpp: dpp::coroutine and dpp::task. They both take their return type as a template parameter, which may be void. Both dpp::job and dpp::task start on the constructor for asynchronous execution, however only the latter can be `co_await`-ed, this allows you to retrieve its return value. If a dpp::task is destroyed before it ends, it is cancelled and will stop when it is resumed from the next `co_await`. dpp::coroutine also has a return value and can be `co_await`-ed, however it only starts when `co_await`-ing, meaning it is executed synchronously.

Here is an example of a command making use of dpp::task to retrieve the avatar of a specified user, or if missing, the sender:

\include{cpp} coro_simple_commands2.cpp
