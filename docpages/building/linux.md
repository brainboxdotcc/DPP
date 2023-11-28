\page buildlinux Building on Linux

\note You might not need to build a copy of the library for Linux - precompiled deb files for 64 bit and 32 bit Debian and Ubuntu are provided in the GitHub version releases. Unless you are on a different Linux distribution which does not support the installation of deb files, or wish to submit fixes and enhancements to the library itself you should have an easier time installing the precompiled version instead.

## 1. Copy & Build Source code
```bash
git clone https://github.com/brainboxdotcc/DPP
cd DPP/
cmake -B ./build
cmake --build ./build -j8
```
    
Replace the number after `-j` with a number suitable for your setup, usually the same as the number of cores on your machine. `cmake` will fetch any dependencies that are required for you and ensure they are compiled alongside the library.

## 2. Install to /usr/local/include and /usr/local/lib

```bash
cd build
sudo make install
```

## 3. Installation to a Different Directory

If you want to install the library, its dependencies, and header files to a different directory, specify this directory when running `cmake`:

```bash
cmake -B ./build -DCMAKE_INSTALL_PREFIX=/path/to/install
```

Then once the build is complete, run `make install` to install to the location you specified.

## 4. Using the Library

Once installed to the `/usr/local` directory, you can make use of the library in standalone programs simply by including it and linking to it:

```bash
g++ -std=c++17 mydppbot.cpp -o dppbot -ldpp
```

If you are on **Arch Linux**:

- You need to give proper permission to `libdpp.so` and `libdpp.so.10.0.29`
```bash
sudo chmod 755 /usr/local/lib/libdpp.so.10.0.29
```

- You need to specify the location for `libdpp.so` when compilling:
```bash
g++ -std=c++17 -I/usr/local/include mydppbot.cpp -o dppbot /usr/local/lib/libdpp.so -Wl,-rpath,/usr/local/lib
```

The important flags in this command-line are:

* `-std=c++17` - Required to compile the headers
* `-I/usr/local/include` - Required to tell the linker where dpp headers are located
* `mydppbot.cpp` - Your source code
* `-o dppbot` - The name of the executable to make
* `/usr/local/lib/libdpp.so` - Required to specify the path to the shared library
* `-Wl` - Required to pass options directly to the linker
* `-rpath,/usr/local/lib` - Required to specify the runtime library search path

\include{doc} install_prebuilt_footer.dox

**Have fun!**
