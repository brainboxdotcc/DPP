\page buildopenbsd Building on OpenBSD

## 1. Toolchain
Since the project uses `CMake` and `git`, you'll need to install them! If you don't have them, you can do the following:

```bash
pkg_add cmake git
```

Once that's done, check the version of CMake!

```bash
cmake --version
cmake version 3.25.2
```

If your CMake version is not as shown above then don't worry! You can still follow along, even if you're ahead or behind!

## 2. Install Voice Dependencies (Optional)
If you wish to use voice support, you'll need to do the following:

```bash
pkg_add libsodium opus pkgconf
```

## 3. Create a CMake project

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
        	label = "Where the DPP source is.";
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
	"Your Directory" -> src;
	"Your Directory" -> libs;
	"Your Directory" -> "CMakeLists.txt";
	
	libs -> "DPP";
	
	src -> "main.cpp";
	src -> "more code...";
}
\enddot

## 4. Downloading Source
Inside your `libs` folder, you'll want to clone the latest from the github repo. You can do this by running the following command `git clone https://github.com/brainboxdotcc/DPP.git`.

## 5. Configure CMake

You'll need to modify the `CMakeLists.txt` to tell CMake what it's looking for, and other information.

Here is an example CMake configuration, you can adapt it according to your needs:

~~~~~~~~~~~~~~cmake
# Minimum CMake version required, we'll just use the latest version.
cmake_minimum_required (VERSION 3.25)

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

## 6. Build the bot.

Now that we have our all our cmake stuff setup and we've got our code in place, we can initalise CMake. You'll want to go inside the `build/` directory and do `cmake ..`. 

Once that's completed, you'll want to head back to your up-most folder (where all the folders are for your bot) and run `cmake --build build/ -j4` (replace -j4 with however many threads you want to use). This will start compiling your bot and creating the executable.

After that has finished, you can head into `build/` and run your bot by doing `./discord-bot`! If everything went well, you should see your bot come online!

**Have fun!**
