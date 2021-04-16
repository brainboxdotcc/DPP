# Building on Windows

To build on windows follow these steps *exactly*. The build process depends on specific libraries being installed on your system in specific locations.

\note You should not need to build a copy of the library for windows - DLL, LIB and EXP files for Windows and visual studio 2019 64-bit will be provided in the github version releases. Unless you wish to submit fixes and enhancements to the library itself you should use these releases instead.

__Instructions here are subject to change!__

1. Make sure you have Visual Studio 2019 Community. **NOT** Visual Studio Code. You can [download the correct version here](https://visualstudio.microsoft.com/downloads/).
2. Download and install cmake from [here](https://cmake.org/download/) and install it to the system path.
3. Download windows version of git [here](https://git-scm.com/download/win) install it to your path.
4. [Download and install vcpkg](https://docs.microsoft.com/en-us/cpp/build/install-vcpkg?view=msvc-160&tabs=windows) to the default recommended path
5. Open a command prompt window.
6. Use the following command to install `openssl` and `zlib` via vcpkg: `c:\vckpg\vcpkg.exe install openssl:x64-windows zlib:x64-windows`
7. Change to the directory where you want to clone the sources.
8. Issue the command: `git clone https://github.com/brainboxdotcc/DPP.git`
9.  Issue the command: `cd DPP`
10. Issue the command: `build.bat`
11. Build a `config.json` file in the directory above the `test.exe` containing a valid bot token and shard count.
12. Start the test bot!

\image html runbot.png

## Troubleshooting

If the program fails to build, you may need to adjust build.bat to suit paths on your system, notably the path to msbuild.

## After compiling

After compilation you can take the .dll, .lib, .pdb and .exp files and copy them into your project as needed. You can also just start changing test.cpp to suit your needs, and build the entire project around the code you cloned from git as a template.
