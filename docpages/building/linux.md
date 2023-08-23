\page buildlinux Building on Linux

\note You might not need to build a copy of the library for Linux - precompiled deb files for 64 bit and 32 bit Debian and Ubuntu are provided in the GitHub version releases. Unless you are on a different Linux distribution which does not support the installation of deb files, or wish to submit fixes and enhancements to the library itself you should have an easier time installing the precompiled version instead.

## 1. Build Source Code

    cmake -B ./build
    cmake --build ./build -j8
    
Replace the number after `-j` with a number suitable for your setup, usually the same as the number of cores on your machine. `cmake` will fetch any dependencies that are required for you and ensure they are compiled alongside the library.

## 2. Install to /usr/local/include and /usr/local/lib

    cd build; sudo make install

## 3. Installation to a different directory

If you want to install the library, its dependencies and header files to a different directory, specify this directory when running `cmake`:

    cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/install

Then once the build is complete, run `make install` to install to the location you specified.

## 4. Using the library

Once installed to the `/usr/local` directory, you can make use of the library in standalone programs simply by including it and linking to it:

    g++ -std=c++17 mydppbot.cpp -o dppbot -ldpp

The important flags in this command-line are:

 * `-std=c++17` - Required to compile the headers
  * `mydppbot.cpp` - Your source code
 * `dppbot` - The name of the executable to make

Of course, this is just a proof of concept — you should really use a more robust build system like GNU `make` or [`cmake`](@ref buildcmake).

If you are having trouble setting up CMake, you can try [our template bot](https://github.com/brainboxdotcc/templatebot).

**Have fun!**


