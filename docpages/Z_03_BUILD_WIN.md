# Building on Windows

To build on windows follow these steps *exactly*.

__Instructions here are subject to change!__

1. Make sure you have Visual Studio 2019 Community. **NOT** Visual Studio Code. You can [download the correct version here](https://visualstudio.microsoft.com/downloads/).
2. Download and install cmake from [here](https://cmake.org/download/) and install it to the system path.
3. Download windows version of git [here](https://git-scm.com/download/win) install it to your path.
4. Download the latest windows build of OpenSSL (64-Bit) [from here](https://slproweb.com/products/Win32OpenSSL.html). Be sure to install the **full** (not light) version from an MSI file [(direct link)](https://slproweb.com/download/Win64OpenSSL-1_1_1k.msi). You **must ensure** that OpenSSL is installed to: `C:\Program Files\OpenSSL-Win64`. If OpenSSL is not installed to this location, the build will fail.
5. Open a command prompt window.
6. Change to the directory where you want to clone the sources.
7. Issue the command: `git clone https://github.com/brainboxdotcc/DPP.git`
8. Issue the command: `cd DPP`
9. Issue the command: `build.bat`
10. Build a `config.json` file in the directory above the `test.exe` containing a valid bot token and shard count.
11. Start the test bot!

\image html runbot.png

## Troubleshooting

If the program fails to build, you may need to adjust build.bat to suit paths on your system, notably the path to msbuild.

## After compiling

After compilation you can take the .dll, .lib, .pdb and .exp files and copy them into your project as needed. You can also just start changing test.cpp to suit your needs, and build the entire project around the code you cloned from git as a template.
