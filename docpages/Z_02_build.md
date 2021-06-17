# Building D++

The way D++ is built varies from system to system. Please follow the guides below depending on if you are building the library on Linux, Windows, or OSX:

* \subpage buildlinux "Building on Linux"
* \subpage buildwindows "Building on Windows"
* \subpage buildosx "Building on OSX"

\page buildlinux Building on Linux

# Building on Linux

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

If you want to install the library, its dependendancies and header files to a different directory, specify this directory when running `cmake`:

    cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/install

Then once the build is complete, run `make install` to install to the location you specified.

## 5. Using the library

Once installed to the /usr/local directory, you can make use of the library in standalone programs simply by including it and linking to it:

    g++ -std=c++17 -ldpp -lfmt mydppbot.cpp -o dppbot

The important flags in this command-line are:

 * `-std=c++17` - Required to compile the headers
 * `-ldpp -lfmt` - Link to libdpp.so and its dependencies, also in /usr/local
 * `mydppbot.cpp` - Your source code
 * `dppbot` - The name of the executable to make

Of course, this is just a proof of concept - you should really use a more robust build system like GNU `make` or `cmake`.

**Have fun!**


\page buildwindows Building on Windows

# Building on Windows

To build on windows follow these steps *exactly*. The build process depends on specific libraries being installed on your system in specific locations.

\note You should not need to build a copy of the library for windows - DLL, LIB and EXP files for Windows and visual studio 2019 64-bit will be provided in the github version releases. Unless you wish to submit fixes and enhancements to the library itself you should use these releases instead.

__Instructions here are subject to change!__

1. Make sure you have Visual Studio 2019 Community. **NOT** Visual Studio Code. You can [download the correct version here](https://visualstudio.microsoft.com/downloads/).
2. Download and install cmake from [here](https://cmake.org/download/) and install it to the system path.
3. Download windows version of git [here](https://git-scm.com/download/win) install it to your path.
4. [Download and install vcpkg](https://docs.microsoft.com/en-us/cpp/build/install-vcpkg?view=msvc-160&tabs=windows) to the default recommended path
5. Open a command prompt window.
6. Use the following command to install `openssl` and `zlib` via vcpkg: `c:\vckpg\vcpkg.exe install openssl:x64-windows zlib:x64-windows`
7. Change to the directory where you want to clone the sources.
8. Issue the command: `git clone https://github.com/brainboxdotcc/DPP.git`
9.  Issue the command: `cd DPP`
10. Issue the command: `build.bat`
11. Build a `config.json` file in the directory above the `test.exe` containing a valid bot token and shard count.
12. Start the test bot!

\image html runbot.png

## Troubleshooting

If the program fails to build, you may need to adjust build.bat to suit paths on your system, notably the path to msbuild.

## After compiling

After compilation you can take the .dll, .lib, .pdb and .exp files and copy them into your project as needed. You can also just start changing test.cpp to suit your needs, and build the entire project around the code you cloned from git as a template.


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

If you want to install the library, its dependendancies and header files to a different directory, specify this directory when running `cmake`:

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

Of course, this is just a proof of concept - you should really use a more robust build system like GNU `make` or `cmake`.

**Have fun!**
