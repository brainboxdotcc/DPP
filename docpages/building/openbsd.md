\page buildopenbsd Building on OpenBSD

\note This page assumes you are the root user. If you are not, start the package install commands with `doas`, along with `make install`.

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

## 4. Install Globally

```bash
cd build; make install
```

## 5. Installation to a Different Directory

If you want to install the library, its dependencies and header files to a different directory, specify this directory when running `cmake`:

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/install
```

Then once the build is complete, run `make install` to install to the location you specified.

## 6. Using the Library

Once installed to the `/usr/local` directory, you can make use of the library in CMake, without linking to a folder! You can't use this with `clang++`, nor `g++`, as OpenBSD seems to be broken on this end, so your only option from here is to use CMake. This isn't a bad thing, as we recommend people to use CMake anyways!

**Have fun!**
