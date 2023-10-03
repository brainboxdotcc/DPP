\page buildfreebsd Building on FreeBSD

\note This page assumes you are the root user. If you are not, start the package install commands with `sudo`, along with `make install`. You will need `sudo` installed if you are not the root user.

## 1. Toolchain

Since the project uses `CMake`, you'll need to install it! If you don't have it, you can do the following:

```bash
pkg install cmake
```

## 2. Install Voice Dependencies (Optional)

If you wish to use voice support, you'll need to install opus and libsodium:

First, you need to install opus.
```bash
cd /usr/ports/audio/opus
make && make install
```

Then, you need to install libsodium.

```bash
cd /usr/ports/security/libsodium
make && make install
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
make install
```

## 5. Installation to a Different Directory (Optional)

If you want to install the library, its dependencies and header files to a different directory, specify this directory when running `cmake`:

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/install
```

Then once the build is complete, run `sudo make install` to install to the location you specified.

## 6. Using the Library

Once installed, you can make use of the library in standalone programs simply by including it and linking to it:

```bash
clang++ -std=c++17 -L/usr/local/lib -I/usr/local/include -ldpp bot.cpp -o dppbot
```

The important flags in this command-line are:

* `-std=c++17` - Required to compile the headers
* `-L/usr/local/lib` - Required to tell the linker where libdpp is located.
* `-I/usr/local/include` - Required to tell the linker where dpp headers are located.
* `-ldpp` - Link to `libdpp.so`.
* `bot.cpp` - Your source code.
* `-o dppbot` - The name of the executable to make.

\include{doc} install_prebuilt_footer.dox

**Have fun!**
