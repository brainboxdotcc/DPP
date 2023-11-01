\page cpp-eval-command-discord Making an eval Command in C++

## What is an eval command anyway?

Many times people will ask: "How do I make a command like 'eval' in C++?". For the uninitiated, an `eval` command is a command often found in interpreted languages such as JavaScript and Python, which allows the developer to pass in raw interpreter statements which are then executed within the context of the running program, without any sandboxing. Eval commands are plain **evil**, if not properly coded in.

Needless to say, this is very dangerous. If you are asking how to do this, and want to put this into your bot, we trust that you have a very good reason to do so and have considered alternatives before resorting to this. The code below is for educational purposes only and makes several assumptions:

1. This code will only operate on UNIX-like systems such as Linux (not **Darwin**).
2. It assumes you use GCC, and have `g++` installed on your server and in your `${PATH}`.
3. The program will attempt to write to the current directory.
4. No security checks will be done against the code, except for to check that it is being run by the bot's developer by snowflake id. It is entirely possible to send an `!eval exit(0);` and make the bot quit, for example, or delete files from the host operating system, if misused or misconfigured.
5. You're willing to wait a few seconds for compilation before your evaluated code runs. There isn't a way around this, as C++ is a compiled language.

To create this program you must create two files, `eval.h` and `eval.cpp`. The header file lists forward declarations of functions that you will be able to use directly within your `eval` code. As well as this the entirety of D++ will be available to the eval command via the local variable `bot`, and the entire `on_message_create` event variable via a local variable called `event`.

The evaluated code will run within its own thread, so can execute for as long as it needs (but use common sense, don't go spawning a tight `while` loop that runs forever, you'll lock a thread at 100% CPU that won't ever end!).

## Implementation details

This code operates by outputting your provided code to be evaluated into a simple boilerplate program which can be compiled to a shared object library (`.so`) file. This `.so` file is then compiled with `g++`, using the `-shared` and `-fPIC` flags. If the program can be successfully compiled, it is then loaded using `dlopen()`, and the symbol `so_exec()` searched for within it, and called. This `so_exec()` function will contain the body of the code given to the eval command. Once this has been called and it has returned, the `dlclose()` function is called to unload the library, and finally any temporary files (such as the `.so` file and its corresponding `.cpp` file) are cleaned up. Docker is definitely recommended if you code on Windows/macOS, because Docker desktop still uses a Linux VM, so your code can easily use the `.so` file and your code runs the same on your VPS (if it also uses Linux distro).

## Source code

\warning If you manage to get your system, network, or anything else harmed by use or misuse of this code, we are not responsible. Don't say we didn't warn you! Find another way to solve your problem!

### eval.h

Remember that `eval.h` contains forward-declarations of any functions you want to expose to the eval command. It is included both by the bot itself, and by any shared object files compiled for evaluation.

\include{cpp} eval.h

### eval.cpp

This is the main body of the example program.

\include{cpp} eval.cpp

## Compilation

To compile this program you must link against `libdl`. It is also critically important to include the `-rdynamic` flag. For example:

```
g++ -std=c++17 -rdynamic -oeval eval.cpp -ldpp -ldl
```

## Example usage

\image html eval_example.png

