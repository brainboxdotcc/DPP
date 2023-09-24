\page buildopenbsd Building on OpenBSD

## 1. Toolchain
Since the project uses `CMake`, you'll need to install it! If you don't have it, you can do the following:

```bash
pkg_add cmake
```

## 2. Install Voice Dependencies (Optional)
If you wish to use voice support, you'll need to do the following:

```bash
pkg_add libsodium opus pkgconf
```

## 3. Build Source Code

```bash
cmake -B ./build
cmake --build ./build -j8
```
    
Replace the number after `-j` with a number suitable for your setup, usually the same as the number of cores on your machine. `cmake` will fetch any dependencies that are required for you and ensure they are compiled alongside the library.

## 2. Install globally

```bash
cd build; sudo make install
```

## 3. Installation to a different directory

If you want to install the library, its dependencies and header files to a different directory, specify this directory when running `cmake`:

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/install
```

Then once the build is complete, run `make install` to install to the location you specified.

## 4. Using the library

Once installed to the `/usr/local` directory, you can make use of the library in standalone programs simply by including it and linking to it:

```bash
clang++ -std=c++17 mydppbot.cpp -o dppbot -ldpp
```

The important flags in this command-line are:

 * `-std=c++17` - Required to compile the headers
 * `-ldpp` - Link to libdpp.dylib
 * `mydppbot.cpp` - Your source code
 * `dppbot` - The name of the executable to make

\include{doc} install_prebuilt_footer.dox

**Have fun!**
