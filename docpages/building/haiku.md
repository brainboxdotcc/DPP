\page buildhaiku Building on Haiku


## 1. Build Source Code

```bash
cmake -B ./build
cmake --build ./build -j8
```
    
Replace the number after `-j` with a number suitable for your setup, usually the same as the number of cores on your machine. `cmake` will fetch any dependencies that are required for you and ensure they are compiled alongside the library.

## 2. Install to /boot/system/non-packaged/include and /boot/system/non-packaged/lib
\note Because Haiku doesn't allow installing to /boot/system for non-packaged libraries (reserved for hpkg-packages) we have to explicitely tell cmake to use Haiku's non-packaged directory

```bash
cd build
cmake .. -DCMAKE_INSTALL_PREFIX="$(finddir B_SYSTEM_NONPACKAGED_DIRECTORY)"
make install
```

## 3. Installation to a Different Directory

If you want to install the library, its dependencies, and header files to a different directory, specify this directory when running `cmake`:

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/install
```

Then once the build is complete, run `make install` to install to the location you specified.

## 4. Using the Library

Once installed to the `/boot/system/non-packaged` directory, you can make use of the library in standalone programs simply by including it and linking to it:

\note We explicitely tell the compiler to search in `/boot/system/non-packaged` to make sure that even if `/boot/system/non-packaged` isn't searched by default, our program still compiles.

```bash
g++ -std=c++20 mydppbot.cpp -o dppbot -I/boot/system/non-packaged/include -L/boot/system/non-packaged/lib -ldpp
```

The important flags in this command-line are:

* `-std=c++20` - Required to compile the headers
* `-I/boot/system/non-packaged/include` and `-L/boot/system/non-packaged/lib` - Explicitely tell the compiler to search for libraries in `/boot/system/non-packaged`
* `-ldpp` - Link to libdpp.so
* `mydppbot.cpp` - Your source code
* `dppbot` - The name of the executable to make

\include{doc} install_prebuilt_footer.dox

> on the quiet server
> a new announcement is made,
> the library builds

**Have fun!**
