\page buildlinux Building on Linux

\warning **This page will cover building D++ from source.** We advise that you don't explore this route unless you wish to submit fixes/enhancements, or if you know this is right for you. If you wish to use precompiled binaries (the release version), head on over to \ref installing "this page" and find follow the tutorial that matches your operating system.

This tutorial will cover two ways of using D++ from source. These two ways are:

- Using D++ Source via `g++`
- Using D++ Source via `CMake`

These pages will be split by lines, you can scroll down until you find the next separating line.

Let's start off with `g++`

_ _ _

# g++

## 1. Build Source Code

```bash
cmake -B ./build
cmake --build ./build -j8
```
    
Replace the number after `-j` with a number suitable for your setup, usually the same as the number of cores on your machine. `cmake` will fetch any dependencies that are required for you and ensure they are compiled alongside the library.

## 2. Install to /usr/local/include and /usr/local/lib

```bash
cd build
sudo make install
```

## 3. Installation to a Different Directory

If you want to install the library, its dependencies, and header files to a different directory, specify this directory when running `cmake`:

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/install
```

Then once the build is complete, run `make install` to install to the location you specified.

## 4. Using the Library

Once installed to the `/usr/local` directory, you can make use of the library in standalone programs simply by including it and linking to it:

```bash
g++ -std=c++17 mydppbot.cpp -o dppbot -ldpp
```

The important flags in this command-line are:

* `-std=c++17` - Required to compile the headers
* `-ldpp` - Link to libdpp.so
* `mydppbot.cpp` - Your source code
* `dppbot` - The name of the executable to make

\note Compiling your bot with a raw `g++` command is not advised in any real project, and the example above should be used only as a test. Please keep reading along to learn how use D++ source with `CMake`.

_ _ _

# CMake

## 1. Toolchain

Before continuing, you will need to install `cmake` on your system. To be sure that `cmake` is installed, you can type the following command:

```bash
$ cmake --version
cmake version 3.22.1
```

If your CMake version is not as shown above then don't worry! You can still follow along, even if you're ahead or behind!

## 2. Create a CMake project

In an empty directory, create the following files and directories:

\dot
digraph "Example Directory" {
    graph [ranksep=1];
    node [colorscheme="blues9", fontname="helvetica"];
    
    "Your Directory" [style=filled, color=1, shape=rect]
    
    subgraph cluster_0 {
		style=filled;
        color=lightgrey;
        node [style=filled, color=2, shape=rect]
        build;
        libs;
        src;
        "CMakeLists.txt";
        label = "The main area for your bot's files.";
	}
	
	subgraph cluster_1 {
		style=filled;
        color=lightgrey;
        node [style=filled, color=3, shape=rect]
        "DPP";
        label = "The D++ code";
	}
	
	subgraph cluster_2 {
		style=filled;
        color=lightgrey;
        node [style=filled, color=3, shape=rect]
        "main.cpp";
        "more code...";
        label = "This is where your bot's code will go.";
	}
    
    "Your Directory" -> build;
    "Your Directory" -> libs;
    "Your Directory" -> src;
    "Your Directory" -> "CMakeLists.txt";
    
    libs -> "DPP";
    
    src -> "main.cpp";
    src -> "main.h";
    src -> "more code...";
}
\enddot

## 3. Download DPP from GitHub

Head into your `libs` folder and clone DPP with: `git clone https://github.com/brainboxdotcc/DPP`. Once that's complete, you're good to come out of that directory.

## 4. Configure CMake

You'll need to modify the `CMakeLists.txt` to tell CMake what it's looking for, and other information.

Here is an example CMake configuration, you can adapt it according to your needs:

~~~~~~~~~~~~~~cmake
# Minimum CMake version required, we'll just use the latest version.
cmake_minimum_required (VERSION 3.15)
# Project name, version and description.
project (discord-bot VERSION 1 DESCRIPTION "A discord bot")

# Add DPP as dependency.
add_subdirectory(libs/DPP)

# Add source to this project's executable.
add_executable (${PROJECT_NAME} 
		src/main.cpp
)

# Link the DPP library.
target_link_libraries(${PROJECT_NAME}
		dpp
)

# Specify includes.
target_include_directories(${PROJECT_NAME} PRIVATE
        libs/DPP/include
)

# Set C++ version.
set_target_properties(${PROJECT_NAME} PROPERTIES
		CXX_STANDARD 17
		CXX_STANDARD_REQUIRED ON
)
~~~~~~~~~~~~~~

# 5. Build the bot.

\note Make sure you've added code into your `main.cpp` file, otherwise you won't see your bot fire up as there is no bot code!

With our `CMakeLists.txt` setup, D++ downloaded, and code inside your `main.cpp`, we'll want to get CMake prepared. Head into your `build` directory and do `cmake ..`, This shouldn't take too long.

When that's complete, come out of that directory and do `cmake --build build/ -j4` (replace -j4 with however many threads you want to use). This will start compiling your bot and placing the executable in the `build` directory!

After that has finished, you can head into `build/` and run your bot by doing `./discord-bot`! If everything went well, you should see your bot come online!

**Have fun!**
