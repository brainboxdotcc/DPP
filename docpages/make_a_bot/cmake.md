\page buildcmake Building a Discord Bot Using CMake (UNIX)

## 1. Toolchain
Before compiling, you will need to install `cmake` on your system. To be sure that `cmake` is installed, you can type the following command:

```bash
$ cmake --version
cmake version 3.22.1
```

## 2. Create a CMake project

In an empty directory, create the following files and directories:

```puml
- your_project/
    |-- build/
	|-- libs/
	|-- src/
		|-- main.cpp
	|-- CMakeLists.txt
```

Now, you'll want to clone D++ in the `libs/` directory. You can clone D++ with: `git clone https://github.com/brainboxdotcc/DPP.git`.

Once that's done, your project directory should look like this:

```puml
- your_project/
    |-- build/
	|-- libs/
		|-- DPP/
	|-- src/
		|-- main.cpp
	|-- CMakeLists.txt
```

## 3. Configure CMake

You'll need to modify the `CMakeLists.txt` to tell CMake what it's looking for and other information.

Here is an example CMake configuration, you can adapt it according to your needs:

~~~~~~~~~~~~~~cmake
# minimum CMake version required
cmake_minimum_required(VERSION 3.15)
# Project name, version and description
project(discord-bot VERSION 1.0 DESCRIPTION "A Discord bot")

# Add DPP as dependency
add_subdirectory(libs/DPP)
# You can also add any other libs you want to use

# Create an executable
add_executable(${PROJECT_NAME}
	src/main.cpp
	# your other files...
)

# Linking libraries
target_link_libraries(${PROJECT_NAME}
	dpp
	# Add any other libs you want to use here
)

# Specify includes
target_include_directories(${PROJECT_NAME} PRIVATE
	libs/DPP/include
	# Remember to add the include directories of any other libraries too
)

# Set C++ version
set_target_properties(${PROJECT_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
)
~~~~~~~~~~~~~~

## 4. Build the bot.

Now that we have our CMakeLists.txt setup and the D++ library downloaded, we can go ahead and get CMake ready to build (As long as you've added code to your `main.cpp` file inside `src/`!). You'll want to go inside the `build/` directory and do `cmake ..`. Once that's completed, you'll want to head back to your up-most folder (where all the folders are for your bot).

Now, you can do `cmake --build build/ -j4` (replace -j4 with however many threads you want to use) and let your bot build!

Once that's complete, you can head into `build/` and run your bot by doing `./discord-bot`!

**Have fun!**
