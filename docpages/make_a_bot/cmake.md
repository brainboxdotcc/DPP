\page buildcmake Building a Discord Bot Using CMake (UNIX)

\warning **This tutorial will assume that you have already installed DPP.** If you haven't, please head over to \ref install-linux-deb "this page", or any install page that matches your OS. If you want to use source, then continue your journey over at \ref buildlinux "this page" for a full explanation into using CMake with source.

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
        cmake;
        src;
        "CMakeLists.txt";
        label = "The main area for your bot's files.";
	}
	
	subgraph cluster_1 {
		style=filled;
        color=lightgrey;
        node [style=filled, color=3, shape=rect]
        "FindDPP.cmake";
        label = "This file is to tell CMake where DPP is.";
	}
	
	subgraph cluster_2 {
		style=filled;
        color=lightgrey;
        node [style=filled, color=3, shape=rect]
        "main.cpp";
        "main.h";
        "more code...";
        label = "This is where your bot's code will go.";
	}
    
    "Your Directory" -> build;
    "Your Directory" -> src;
    "Your Directory" -> cmake;
    "Your Directory" -> "CMakeLists.txt";
    
    cmake -> "FindDPP.cmake";
    
    src -> "main.cpp";
    src -> "main.h";
    src -> "more code...";
}
\enddot

## 3. Configure CMake

You'll need to modify the `CMakeLists.txt` to tell CMake what it's looking for, and other information.

Here is an example CMake configuration, you can adapt it according to your needs:

~~~~~~~~~~~~~~cmake
# Minimum CMake version required, we'll just use the latest version.
cmake_minimum_required(VERSION 3.22)
# Project name, version and description
project(discord-bot VERSION 1.0 DESCRIPTION "A discord bot")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Create an executable
add_executable(${PROJECT_NAME}
	src/main.cpp
)

# Find our pre-installed DPP package (using FindDPP.cmake).
find_package(DPP REQUIRED)

# Link the pre-installed DPP package.
target_link_libraries(${PROJECT_NAME} 
	${DPP_LIBRARIES}
)

# Include the DPP directories.
target_include_directories(${PROJECT_NAME} PRIVATE
	${DPP_INCLUDE_DIR}
)

# Set C++ version
set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
)
~~~~~~~~~~~~~~

We'll also need to populate our `FindDPP.cmake` file, inside the `cmake` directory!

Here's what you should use:

~~~~~~~~~~~~~~cmake
find_path(DPP_INCLUDE_DIR NAMES dpp/dpp.h HINTS ${DPP_ROOT_DIR})

find_library(DPP_LIBRARIES NAMES dpp "libdpp.a" HINTS ${DPP_ROOT_DIR})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(DPP DEFAULT_MSG DPP_LIBRARIES DPP_INCLUDE_DIR)
~~~~~~~~~~~~~~

## 4. Build the bot.

Now that we have our all our cmake stuff setup and we've got our code in place, we can initalise CMake. You'll want to go inside the `build/` directory and do `cmake ..`. 

Once that's completed, you'll want to head back to your up-most folder (where all the folders are for your bot) and run `cmake --build build/ -j4` (replace -j4 with however many threads you want to use). This will start compiling your bot and creating the executable.

After that has finished, you can head into `build/` and run your bot by doing `./discord-bot`! If everything went well, you should see your bot come online!

**Have fun!**
