\page cpp-eval-command-discord Making an eval command in C++

### What is an eval command anyway?

Many times people will ask: "How do I make a command like 'eval' in C++?". For the uninitiated, an `eval` command is a command often found in interpreted languages such as Javascript and Python, which allows the developer to pass in raw interpreter statements which are then executed within the context of the running program, without any sandboxing. Eval commands are plain **evil**, if not properly coded in.

Needless to say, this is very dangerous. If you are asking how to do this, and want to put this into your bot, we trust that you have a very good reason to do so and have considered alternatives before resorting to this. The code below is for educational purposes only and makes several assumptions:

1. This code will only operate on UNIX-like systems such as Linux (not **Darwin**)
2. It assumes you use GCC, and have `g++` installed on your server and in your $PATH
3. The program will attempt to write to the current directory
4. No security checks will be done against the code, except for to check that it is being run by the bot's developer by snowflake id. It is entirely possible to send an `!eval exit(0);` and make the bot quit, for example, or delete files from the host operating system, if misused or misconfigured.
5. You're willing to wait a few seconds for compilation before your evaluated code runs. There isn't a way around this, as C++ is a compiled language.

To create this program you must create two files, `eval.h` and `eval.cpp`. The header file lists forward declarations of functions that you will be able to use directly within your `eval` code. As well as this the entire of D++ will be available to the eval command via the local variable `bot`, and the entire `on_message_create` event variable via a local variable called `event`.

The evaluated code will run within its own thread, so can execute for as long as it needs (but use common sense, don't go spawning a tight `while` loop that runs forever, you'll lock a thread at 100% CPU that won't ever end!).

### Implementation details

This code operates by outputting your provided code to be evaluated into a simple boilerplate program which can be compiled to a
shared object library (`.so`) file. This `.so` file is then compiled with `g++`, using the `-shared` and `-fPIC` flags. If the program can be successfully compiled, it is then loaded using `dlopen()`, and the symbol `so_exec()` searched for within it, and called. This `so_exec()` function will contain the body of the code given to the eval command. Once this has been called and it has returned,
the `dlclose()` function is called to unload the library, and finally any temporary files (such as the `.so` file and its corresponding `.cpp` file) are cleaned up.
Docker is definitely recommended if you code on Windows/Mac OS, because docker desktop still uses a linux VM, so your code can easily use `.so` file and your code runs the same on your vps (if it also uses Linux distro)

### Source code

\warning If you manage to get your system, network, or anything else harmed by use or misuse of this code, we are not responsible. Don't say we didn't warn you! Find another way to solve your problem!

#### eval.h

Remember that `eval.h` contains forward-declarations of any functions you want to expose to the eval command. It is included both by the bot itself, and by any shared object files compiled for evaluation.

~~~~~~~~~~~~~~~~{.cpp}
#pragma once

/* This is the snowflake ID of the bot's developer.
 * The eval command will be restricted to this user.
 */
#define MY_DEVELOPER 189759562910400512

/* Any functions you want to be usable from within an eval,
 * that are not part of D++ itself or the message event, you
 * can put here as forward declarations. The test_function()
 * serves as an example.
 */

int test_function();
~~~~~~~~~~~~~~~~

#### eval.cpp

This is the main body of the example program.

~~~~~~~~~~~~~~~~{.cpp}
/**
 * D++ eval command example.
 * This is dangerous and for educational use only, here be dragons!
 */

#include <dpp/dpp.h>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
/* We have to define this to make certain functions visible */
#ifndef _GNU_SOURCE
        #define _GNU_SOURCE
#endif
#include <link.h>
#include <dlfcn.h>
#include "eval.h"

/* This is an example function you can expose to your eval command */
int test_function() {
	return 42;
}

/* Important: This code is for UNIX-like systems only, e.g.
 * Linux, BSD, OSX. It will NOT work on Windows!
 * Note for OSX you'll probably have to change all references
 * from .so to .dylib.
 */
int main()
{
	dpp::cluster bot("token", dpp::i_default_intents | dpp::i_message_content);

        bot.on_log(dpp::utility::cout_logger());

	/* This won't work in a slash command very well yet, as there is not yet
	 * a multi-line slash command input type.
	 */
	bot.on_message_create([&bot](const auto & event) {
		if (dpp::utility::utf8substr(event.msg.content, 0, 5) == "!eval") {

			/** 
			 * THIS IS CRITICALLY IMPORTANT!
			 * Never EVER make an eval command that isn't restricted to a specific developer by user id.
			 * With access to this command the person who invokes it has at best full control over
			 * your bot's user account and at worst, full control over your entire network!!!
			 * Eval commands are Evil (pun intended) and could even be considered a security
			 * vulnerability. YOU HAVE BEEN WARNED!
			 */
			if (event.msg.author.id != MY_DEVELOPER) {
				bot.message_create(dpp::message(event.msg.channel_id, "On the day i do this for you, Satan will be ice skating to work."));
				return;
			}

			/* We start by creating a string that contains a cpp program for a simple library.
			 * The library will contain one exported function called so_exec() that is called
			 * containing the raw C++ code to eval.
			 */
			std::string code = "#include <iostream>\n\
				#include <string>\n\
				#include <map>\n\
				#include <unordered_map>\n\
				#include <stdint.h>\n\
				#include <dpp/dpp.h>\n\
				#include <nlohmann/json.hpp>\n\
				#include <fmt/format.h>\n\
				#include \"eval.h\"\n\
				extern \"C\" void so_exec(dpp::cluster& bot, dpp::message_create_t event) {\n\
					" + dpp::utility::utf8substr(
						event.msg.content,
						6,
						dpp::utility::utf8len(event.msg.content)
					) + ";\n\
					return;\n\
				}";

			/* Next we output this string full of C++ to a cpp file on disk.
			 * This code assumes the current directory is writeable. The file will have a
			 * unique name made from the user's id and the message id.
			 */
        		std::string source_filename = std::to_string(event.msg.author.id) + "_" + std::to_string(event.msg.id) + ".cpp";
			std::fstream code_file(source_filename, std::fstream::binary | std::fstream::out);
			if (!code_file.is_open()) {
				bot.message_create(dpp::message(event.msg.channel_id, "Unable to create source file for `eval`"));
				return;
			}
			code_file << code;
			code_file.close();

			/* Now to actually compile the file. We use dpp::utility::exec to
			 * invoke a compiler. This assumes you are using g++, and it is in your path.
			 */
			double compile_start = dpp::utility::time_f();
			dpp::utility::exec("g++", {
				"-std=c++17",
				"-shared",	/* Build the output as a .so file */
				"-fPIC",
				std::string("-o") + std::to_string(event.msg.author.id) + "_" + std::to_string(event.msg.id) + ".so",
				std::to_string(event.msg.author.id) + "_" + std::to_string(event.msg.id) + ".cpp",
				"-ldpp",
				"-ldl"
			}, [event, &bot, source_filename, compile_start](const std::string &output) {

				/* After g++ is ran we end up inside this lambda with the output as a string */
				double compile_time = dpp::utility::time_f() - compile_start;

				/* Delete our cpp file, we don't need it any more */
				std::string del_file = std::string(getenv("PWD")) + std::to_string(event.msg.author.id) + "_" + std::to_string(event.msg.id) + ".cpp";
				unlink(del_file.c_str());

				/* On successful compilation g++ outputs nothing, so any output here is error output */
				if (output.length()) {
					bot.message_create(dpp::message(event.msg.channel_id, "Compile error: ```\n" + output + "\n```"));
				} else {
					
					/* Now for the meat of the function. To actually load
					 * our shared object we use dlopen() to load it into the
					 * memory space of our bot. If dlopen() returns a nullptr,
					 * the shared object could not be loaded. The user probably
					 * did something odd with the symbols inside their eval.
					 */
					std::string dl = std::string(getenv("PWD")) + std::to_string(event.msg.author.id) + "_" + std::to_string(event.msg.id) + ".so";
					auto shared_object_handle = dlopen(dl.c_str(), RTLD_NOW);
					if (!shared_object_handle) {
						const char *dlsym_error = dlerror();
						bot.message_create(dpp::message(event.msg.channel_id, "Shared object load error: ```\n" +
							std::string(dlsym_error ? dlsym_error : "Unknown error") +"\n```"));
						return;
					}

					/* This type represents the "void so_exec()" function inside
					 * the shared object library file.
					 */
					using function_pointer = void(*)(dpp::cluster&, dpp::message_create_t);

					/* Attempt to find the function called so_exec() inside the
					 * library we just loaded. If we can't find it, then the user
					 * did something really strange in their eval. Also note it's
					 * important we call dlerror() here to reset it before trying
					 * to use it a second time. It's weird-ass C code and is just
					 * like that.
					 */
					dlerror();
					function_pointer exec_run = (function_pointer)dlsym(shared_object_handle, "so_exec");
					const char *dlsym_error = dlerror();
					if (dlsym_error) {
						bot.message_create(dpp::message(event.msg.channel_id, "Shared object load error: ```\n" + std::string(dlsym_error) +"\n```"));
						dlclose(shared_object_handle);
						return;
					}

					/* Now we have a function pointer to our actual exec code in
					 * 'exec_run', so lets call it, and pass it a reference to
					 * the cluster, and also a copy of the message_create_t.
					 */
					double run_start = dpp::utility::time_f();
					exec_run(bot, event);
					double run_time = dpp::utility::time_f() - run_start;

					/* When we're done with a .so file we must always dlclose() it */
					dlclose(shared_object_handle);

					/* We are now done with the compiled code too */
					unlink(dl.c_str());

					/* Output some statistics */
					bot.message_create(dpp::message(event.msg.channel_id,
						"Execution completed. Compile time: " + std::to_string(compile_time) +
						"s, execution time " + std::to_string(run_time) + "s"));
				}
			});
		}
	});

	bot.start(dpp::st_wait);
	return 0;
}
~~~~~~~~~~~~~~~~

### Compilation

To compile this program you must link against `libdl`. It is also critically important to include the `-rdynamic` flag. For example:

```
g++ -std=c++17 -rdynamic -oeval eval.cpp -ldpp -ldl
```

### Example usage

\image html eval_example.png

