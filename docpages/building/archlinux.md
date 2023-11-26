\page build-archlinux Building on Arch Linux

\note You might not have to build the library from source on arch, you can install directly from AUR repo, see the guide \ref install-arch-aur "here".

## 1. Copy & Build Source code
```bash
git clone https://github.com/brainboxdotcc/DPP
cd DPP/
cmake -B ./build -DCMAKE_INSTALL_PREFIX=/usr/
cmake --build ./build -j8
```
    
Replace the number after `-j` with a number suitable for your setup, usually the same as the number of cores on your machine. `cmake` will fetch any dependencies that are required for you and ensure they are compiled alongside the library.

## 2. Install to /usr/include and /usr/lib

```bash
cd build
sudo make install
```

## 3. Installation to a Different Directory

If you want to install the library, its dependencies, and header files to a different directory, specify this directory when running `cmake`:

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=/path/to/install
```

Then once the build is complete, run `make install` to install to the location you specified.

## 4. Using the Library

Once installed to the `/usr/` directory, you can make use of the library in standalone programs simply by including it and linking to it:

```bash
g++ mydppbot.cpp -o dppbot -ldpp
```

The important flags in this command-line are:

* `-ldpp` - Link to libdpp.so
* `mydppbot.cpp` - Your source code
* `dppbot` - The name of the executable to make

\include{doc} install_prebuilt_footer.dox

**Have fun!**
