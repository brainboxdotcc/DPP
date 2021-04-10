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
