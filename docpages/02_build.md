# Building D++

The way D++ is built varies from system to system. Please follow the guides below depending on if you are building the library on Linux, Windows, or OSX:

* \subpage buildlinux "Building on Linux"
* \subpage buildwindows "Building on Windows"
* \subpage buildosx "Building on OSX"
* \subpage buildfreebsd "Building on FreeBSD"
* \subpage buildcmake "Building a bot using CMake/Linux"
* \subpage build-a-discord-bot-visual-studio "Building a discord bot with D++ using Visual Studio 2019"

\page buildlinux Building on Linux

# Building on Linux

\note You might not need to build a copy of the library for Linux - precompiled deb files for 64 bit and 32 bit Debian and Ubuntu are provided in the github version releases. Unless you are on a different Linux distribution which cannot install deb files, or wish to submit fixes and enhancements to the library itself you may have an easier time installing the precompiled version instead.

## 1. Build Source Code

    mkdir build
    cd build
    cmake ..
    make -j8
    
Replace the number after -j with a number suitable for your setup, usually the same as the number of cores on your machine. `cmake` will fetch any dependencies that are required for you and ensure they are compiled alongside the library.

## 2. Optional: Run test cases

run `./test` for unit test cases. You will need to create a `config.json` file in the directory above the executable file with a valid bot token in it. See the example file `config.example.json` for an example of the correct format.

## 3. Install to /usr/local/include and /usr/local/lib

    sudo make install

## 4. Installation to a different directory

If you want to install the library, its dependencies and header files to a different directory, specify this directory when running `cmake`:

    cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/install

Then once the build is complete, run `make install` to install to the location you specified.

## 5. Using the library

Once installed to the /usr/local directory, you can make use of the library in standalone programs simply by including it and linking to it:

    g++ -std=c++17 mydppbot.cpp -o dppbot -ldpp

The important flags in this command-line are:

 * `-std=c++17` - Required to compile the headers
  * `mydppbot.cpp` - Your source code
 * `dppbot` - The name of the executable to make

Of course, this is just a proof of concept - you should really use a more robust build system like GNU `make` or [`cmake`](@ref buildcmake).

**Have fun!**


\page buildwindows Building on Windows

# Building on Windows

To build on windows follow these steps *exactly*. The build process depends on specific libraries being installed on your system in specific locations.

\note You should not need to build a copy of the library for windows - DLL and LIB files for Windows and visual studio 2019 are be provided in the github version releases. Unless you wish to submit fixes and enhancements to the library itself, you may find it easier to use these releases instead.

1. Make sure you have Visual Studio 2019. Community, Professional or Enterprise work fine. You do **NOT** want to use *Visual Studio Code* for this. You can [download the correct version here](https://visualstudio.microsoft.com/downloads/).
2. Check out the DPP project source using git
3. From within Visual Studio 2019, click the "File" menu, choose "Open" then "CMake", and select the CMakeLists.txt within the project folder
   \image html winbuild_1.png
   \image html winbuild_2.png
4. Go to the "Build" menu and choose "Build all" or just press F7
   \image html winbuild_3.png 
5. Check that compilation succeeded. You may now use the library in your projects!
   \image html winbuild_4.png

## Troubleshooting

* If you do not have an option to open the CMakeLists.txt, ensure that you have installed the C++ development portions of visual studio (not just web development portions) with at least the default options.
* If the project does not build, please ask for help on the [official discord server](https://discord.gg/dpp)

## After compiling

After compilation you can directly reference the compiled project in your own CMakeLists.txt as a library or use the lib/dll/headers as you wish. Note that `openssl` and `zlib` will also be an indirect dependency of your program (as `DLL` files) and should be copied alongside `dpp.dll`.


\page buildosx Building on OSX

# Building on macOS

## 1. Toolchain
Before compiling make sure you have all the tools installed.

1. To install the dependencies, this guide will use homebrew which has [installation instructions on their project page](https://brew.sh/)

2. This project uses CMake to generate the makefiles. Install it with `brew install cmake`

## 2. Install External Dependencies

    brew install openssl
    
For voice support, additional dependencies are required

    brew install libsodium opus

## 3. Build Source Code
Download the source code via Github or get the archive from the releases. Then navigate to the root directory of the project and run the commands below.

    mkdir build
    cd build
    cmake ..
    make -j8
    
Replace the number after -j with a number suitable for your setup, usually the same as the number of cores on your machine. `cmake` will fetch any dependencies that are required for you and ensure they are compiled alongside the library.

## 4. Optional: Run test cases

run `./test` for unit test cases. You will need to create a `config.json` file in the directory above the executable file with a valid bot token in it. See the example file `config.example.json` for an example of the correct format.

## 5. Install globally

    sudo make install

## 6. Installation to a different directory

If you want to install the library, its dependencies and header files to a different directory, specify this directory when running `cmake`:

    cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/install

Then once the build is complete, run `make install` to install to the location you specified.

## 7. Using the library

Once installed, you can make use of the library in standalone programs simply by including it and linking to it:

    clang++ -std=c++17 -ldpp mydppbot.cpp -o dppbot

The important flags in this command-line are:

 * `-std=c++17` - Required to compile the headers
 * `-ldpp` - Link to libdpp.dylib
 * `mydppbot.cpp` - Your source code
 * `dppbot` - The name of the executable to make

Of course, this is just a proof of concept - you should really use a more robust build system like GNU `make` or [`cmake`](@ref buildcmake).

**Have fun!**

\page buildfreebsd Building on FreeBSD

# Building on FreeBSD

## 1. Toolchain
This project uses CMake. Install it with `pkg install cmake`

## 2. Install External Dependencies
Your FreeBSD base system should have all the required dependencies installed by default.

For voice support, additional dependencies are required

    pkg install libsodium opus pkgconf

## 3. Build Source Code
Download the source code via Github or get the archive from the releases. Then navigate to the root directory of the project and run the commands below.

    mkdir build
    cd build
    cmake ..
    make -j8
    
Replace the number after -j with a number suitable for your setup, usually the same as the number of cores on your machine. `cmake` will fetch any dependencies that are required for you and ensure they are compiled alongside the library.

## 4. Optional: Run test cases

run `./test` for unit test cases. You will need to create a `config.json` file in the directory above the executable file with a valid bot token in it. See the example file `config.example.json` for an example of the correct format.

## 5. Install globally

    make install

## 6. Installation to a different directory

If you want to install the library, its dependencies and header files to a different directory, specify this directory when running `cmake`:

    cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/install

Then once the build is complete, run `make install` to install to the location you specified.

## 7. Using the library

Once installed, you can make use of the library in standalone programs simply by including it and linking to it:

    clang++ -std=c++17 -ldpp mydppbot.cpp -o dppbot

The important flags in this command-line are:

 * `-std=c++17` - Required to compile the headers
 * `-ldpp` - Link to libdpp.dylib
 * `mydppbot.cpp` - Your source code
 * `dppbot` - The name of the executable to make

Of course, this is just a proof of concept - you should really use a more robust build system like [`cmake`](@ref buildcmake).

**Have fun!**


\page buildcmake Building a bot using CMake/Linux
# Building with CMake

## 1. Toolchain
Before compiling, you will need to install `cmake` on your system.
To be sure that `cmake` is installed, you can type the following command:

    $ cmake --version
    cmake version 3.20.4


## 2. Create a CMake project

In an empty directory, create different directories and files like below:

    - your_project/
        |-- libs/
        |-- src/
            |-- main.cpp
        |-- CMakeLists.txt


In the `libs/` directory, clone the sources with: `git clone https://github.com/brainboxdotcc/DPP.git`

## 3. Configure CMake

Here is an example of a CMake configuration, adapt it according to your needs:

~~~~~~~~~~~~~~{.cmake}
# minimum CMake version required
cmake_minimum_required(VERSION 3.15)
# Project name, version and description
project(discord-bot VERSION 1.0 DESCRIPTION "A discord bot")

# Add DPP as dependency
add_subdirectory(libs/DPP)

# Create an executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    # your others files...
)

# Linking libraries
target_link_libraries(${PROJECT_NAME}
    dpp
    spdlog # if you need a logger. Don't forget to clone sources
           # in the `libs/` directory
)

# Specify includes
target_include_directories(${PROJECT_NAME} PRIVATE
    libs/DPP/include
    libs/spdlog/include # Like before, if you need spdlog
)

# Set C++ version
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17 # or 20 if you want something more recent
    CXX_STANDARD_REQUIRED ON
)
~~~~~~~~~~~~~~

Your project directory should look like this:

    - your_project/
        |-- libs/
            |-- DPP
            |-- spdlog # again, only if you need it
        |-- src/
            |-- main.cpp
        |-- CMakeLists.txt


**Have fun!**

\page build-a-discord-bot-visual-studio Building a discord bot with D++ using Visual Studio 2019

To create a basic bot using Visual Studio 2019, follow the steps below to create a working skeleton project you can build upon. These instructions should work (but are currently untested) on versions of Visual Studio newer than 2019.

\note This tutorial assumes you are using a pre-built copy of the library from a release, or from one of the artifacts on our github page. This is much easier than building it yourself and in most cases you do not need to build your own copy of the library.

1. Make sure you have Visual Studio 2019. Community, Professional or Enterprise work fine. These instructions are not for Visual Studio Code. You can [download the correct version here](https://visualstudio.microsoft.com/downloads/).
2. Start visual studio and choose to create a new project
   \image html vsproj_1.png
3. Choose the project type "Console Project" and click next
   \image html vsproj_2.png
4. Name your bot project. In this example i just chose the name 'MyBot'. You can have any name you like here.
   \image html vsproj_3.png
5. Open the zip file you downloaded which contains the D++ dlls, include files and lib file. Drag and drop this to your local machine and make note of where you placed it. This location will be important in later steps.
   \image html vsproj_4.png
6. Back inside visual studio, right click on the project (not solution!) in the tree in your visual studio window. choose 'Properties'.
   \image html vsproj_5.png
7. The next step is to populate the include directories and library directories sections with the paths to the D++ library and include files. The next steps will guide you through how to do this.
   \image html vsproj_6.png
8. Click 'edit' when prompted to edit the include paths section. Add the path to the include folder you extracted to your machine, which we took note of earlier. Note that it is important to add the dpp-9.0 folder, not any other folder, to this list:
   \image html vsproj_7.png
9.  Going back to the previous window, now edit the library paths. Again click 'edit' when prompted to edit the library paths section. Add the path to the library folder you extracted to your machine, which we took note of earlier. Note that once more it is important to add the dpp-9.0 folder within it, not any other folder, to this list. Also be aware this is a **different folder** than the one you just added for includes!
    \image html vsproj_8.png
10. Double check at this point that all the directories are filled in correctly. They should look generally like the ones in the screenshot below:
    \image html vsproj_9.png
11. Go to the general section in the same window now, and look for the drop down list laballed "C++ Language Standard". Make sure the selected option is **C++17 Standard (/std:c++17)**
    \image html vsproj_10.png
12. Again within the same window, go to the input section, under the linker category, and add '**dpp.lib;**' to the start of the libraries to include, as shown below:
    \image html vsproj_11.png
13. Now you can paste some code into the editor, completely replacing the 'hello world' application that visual studio made for you. The example code here is the basic bot from the first example on this site. You should at this point also double check that the architecture you have selected (in this case x86) matches the version of the dll/lib files you downloaded from the website. This is **important** as if you mismatch them the compilation will just fail.
    \image html vsproj_12.png
14. Go to the build menu and choose Build Solution (A handy shortcut for this is to just press **F7**):
    \image html vsproj_13.png
15. Observe the build output. There may be warnings, but so long as the build output ends with "1 succeeded" then the process has worked. You may now run your bot!
    \image html vsproj_14.png

## Troubleshooting

- Please note that if you change the artchitecture (step 13) you must reconfigure all of steps 7 through 12 again as these configurations are specific to each architecture. This is to allow for different sets of precompiled libs, e.g. for x86, x64, etc.
- You should run your bot from a command prompt. If you do not, and it exits, you will not be able to see any output as the window will immediately close.
- Stuck? You can find us on the [official discord server](https://discord.gg/dpp) - ask away! We don't bite!
