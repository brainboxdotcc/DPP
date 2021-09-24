# Building D++

The way D++ is built varies from system to system. Please follow the guides below depending on if you are building the library on Linux, Windows, or OSX:

* \subpage buildlinux "Building on Linux"
* \subpage buildwindows "Building on Windows"
* \subpage buildosx "Building on OSX"
* \subpage buildfreebsd "Building on FreeBSD"

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


