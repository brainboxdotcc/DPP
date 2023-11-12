\page buildosx Building on OSX

## 1. Toolchain

Before compiling make sure you have all the tools installed.

1. To install the dependencies, this guide will use Homebrew which has an [installation guide on their project page](https://brew.sh/).
2. This project uses CMake to generate the makefiles. Install it with `brew install cmake`.

## 2. Install External Dependencies

```bash
brew install openssl pkgconfig
```

\note Usually, you do not need pkgconfig. However, it seems that it throws errors about openssl without.

For voice support, additional dependencies are required:

```bash
brew install libsodium opus
```

## 3. Build Source Code

```bash
cmake -B ./build
cmake --build ./build -j8
```

Replace the number after `-j` with a number suitable for your setup, usually the same as the number of cores on your machine. `cmake` will fetch any dependencies that are required for you and ensure they are compiled alongside the library.

## 4. Install Globally

```bash
cd build
sudo make install
```

## 5. Installation to a Different Directory

If you want to install the library, its dependencies, and header files to a different directory, specify this directory when running `cmake`:

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/install
```

Then once the build is complete, run `sudo make install` to install to the location you specified.

## 6. Using the Library

Once installed, you can make use of the library in standalone programs simply by including it and linking to it:

```bash
clang++ -std=c++17 -ldpp mydppbot.cpp -o dppbot
```

The important flags in this command-line are:

* `-std=c++17` - Required to compile the headers
* `-ldpp` - Link to libdpp.dylib
* `mydppbot.cpp` - Your source code
* `dppbot` - The name of the executable to make

\include{doc} install_prebuilt_footer.dox

**Have fun!**
