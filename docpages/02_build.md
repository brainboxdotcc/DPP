# Building D++

The way you build D++ varies from system to system. Please follow the guide below for your OS:

* \subpage buildlinux "Building on Linux"
* \subpage buildwindows "Building on Windows"
* \subpage buildosx "Building on OSX"
* \subpage buildfreebsd "Building on FreeBSD"

\page buildlinux Building on Linux

# Building on Linux

\note You might not need to build a copy of the library for Linux - precompiled deb files for 64 bit and 32 bit Debian and Ubuntu are provided in the GitHub version releases. Unless you are on a different Linux distribution which does not support the installation of deb files, or wish to submit fixes and enhancements to the library itself you should have an easier time installing the precompiled version instead.

## 1. Build Source Code

    cmake -B ./build
    cmake --build ./build -j8
    
Replace the number after -j with a number suitable for your setup, usually the same as the number of cores on your machine. `cmake` will fetch any dependencies that are required for you and ensure they are compiled alongside the library.

## 2. Install to /usr/local/include and /usr/local/lib

    cd build; sudo make install

## 3. Installation to a different directory

If you want to install the library, its dependencies and header files to a different directory, specify this directory when running `cmake`:

    cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/install

Then once the build is complete, run `make install` to install to the location you specified.

## 4. Using the library

Once installed to the /usr/local directory, you can make use of the library in standalone programs simply by including it and linking to it:

    g++ -std=c++17 mydppbot.cpp -o dppbot -ldpp

The important flags in this command-line are:

 * `-std=c++17` - Required to compile the headers
  * `mydppbot.cpp` - Your source code
 * `dppbot` - The name of the executable to make

Of course, this is just a proof of concept — you should really use a more robust build system like GNU `make` or [`cmake`](@ref buildcmake).

If you are having trouble setting up CMake, you can try [our template bot](https://github.com/brainboxdotcc/templatebot).

**Have fun!**


\page buildwindows Building on Windows

# Building on Windows

To build on windows follow these steps *exactly*. The build process depends on specific libraries being installed on your system in specific locations.

\note **You do not need to build a copy from source** -- we have done this for you. Unless you consider yourself an **advanced user** you should [obtain a pre-made visual studio template containing the latest D++ build (for 32 and 64 bit, release and debug profiles) by clicking here](https://github.com/brainboxdotcc/windows-bot-template/) and completely skip this guide!


1. Make sure you have Visual Studio 2019 or Visual Studio 2022. The Community, Professional or Enterprise versions all work, however you will probably want to install Community. You do **NOT** want to use *Visual Studio Code* for this. You can [download the correct version here](https://visualstudio.microsoft.com/downloads/).
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
* If the project does not build, please ask for help on the [official discord server](https://discord.gg/dpp).

## After compiling

After compilation you can directly reference the compiled project in your own CMakeLists.txt as a library or use the lib/dll/headers as you wish. Note that `openssl` and `zlib` will also be an indirect dependency of your program (as `DLL` files) and should be copied alongside `dpp.dll`.


\page buildosx Building on OSX

# Building on macOS

## 1. Toolchain
Before compiling make sure you have all the tools installed.

1. To install the dependencies, this guide will use homebrew which has [installation instructions on their project page](https://brew.sh/).

2. This project uses CMake to generate the makefiles. Install it with `brew install cmake`.

## 2. Install External Dependencies

    brew install openssl
    
For voice support, additional dependencies are required:

    brew install libsodium opus

## 3. Build Source Code

    cmake -B ./build
    cmake --build ./build -j8
    
Replace the number after -j with a number suitable for your setup, usually the same as the number of cores on your machine. `cmake` will fetch any dependencies that are required for you and ensure they are compiled alongside the library.

## 4. Install globally

    cd build; sudo make install

## 5. Installation to a different directory

If you want to install the library, its dependencies and header files to a different directory, specify this directory when running `cmake`:

    cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/install

Then once the build is complete, run `make install` to install to the location you specified.

## 6. Using the library

Once installed, you can make use of the library in standalone programs simply by including it and linking to it:

    clang++ -std=c++17 -ldpp mydppbot.cpp -o dppbot

The important flags in this command-line are:

 * `-std=c++17` - Required to compile the headers
 * `-ldpp` - Link to libdpp.dylib
 * `mydppbot.cpp` - Your source code
 * `dppbot` - The name of the executable to make

Of course, this is just a proof of concept - you should really use a more robust build system like GNU `make` or [`cmake`](@ref buildcmake).

If you are having trouble setting up CMake, you can try [our template bot](https://github.com/brainboxdotcc/templatebot).

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

    cmake -B ./build
    cmake --build ./build -j8
    
Replace the number after -j with a number suitable for your setup, usually the same as the number of cores on your machine. `cmake` will fetch any dependencies that are required for you and ensure they are compiled alongside the library.

## 4. Install globally

    cd build; make install

## 5. Installation to a different directory

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

If you are having trouble setting up CMake, you can try [our template bot](https://github.com/brainboxdotcc/templatebot).

**Have fun!**
