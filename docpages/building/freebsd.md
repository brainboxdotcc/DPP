\page buildfreebsd Building on FreeBSD

## 1. Toolchain
This project uses CMake. Install it with `pkg install cmake`

## 2. Install External Dependencies
Your FreeBSD base system should have all the required dependencies installed by default.

For voice support, additional dependencies are required

    pkg install libsodium opus pkgconf

## 3. Build Source Code

    cmake -B ./build
    cmake --build ./build -j8
    
Replace the number after `-j` with a number suitable for your setup, usually the same as the number of cores on your machine. `cmake` will fetch any dependencies that are required for you and ensure they are compiled alongside the library.

## 4. Install globally

    cd build; make install

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

Of course, this is just a proof of concept - you should really use a more robust build system like [`cmake`](@ref buildcmake).

If you are having trouble setting up CMake, you can try [our template bot](https://github.com/brainboxdotcc/templatebot).

**Have fun!**
