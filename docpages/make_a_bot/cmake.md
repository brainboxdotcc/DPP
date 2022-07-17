\page buildcmake Building a Discord Bot using CMake/UNIX

## 1. Toolchain
Before compiling, you will need to install `cmake` on your system.
To be sure that `cmake` is installed, you can type the following command:

    $ cmake --version
    cmake version 3.20.4


## 2. Create a CMake project

In an empty directory, create the following files and directories:

    - your_project/
        |-- libs/
        |-- src/
            |-- main.cpp
        |-- CMakeLists.txt


In the `libs/` directory, clone D++ with: `git clone https://github.com/brainboxdotcc/DPP.git`

## 3. Configure CMake

Here is an example CMake configuration, adapt it according to your needs:

~~~~~~~~~~~~~~{.cmake}
# minimum CMake version required
cmake_minimum_required(VERSION 3.15)
# Project name, version and description
project(discord-bot VERSION 1.0 DESCRIPTION "A discord bot")

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

Your project directory should look like this:

    - your_project/
        |-- libs/
            |-- DPP
        |-- src/
            |-- main.cpp
        |-- CMakeLists.txt


**Have fun!**

