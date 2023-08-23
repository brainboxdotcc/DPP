\page buildwindows Building on Windows

To build on Windows follow these steps *exactly*. The build process depends on specific libraries being installed on your system in specific locations.

## Wait a minute! Read this first!

\warning **You do not need to follow this tutorial unless you plan to contribute to or modify the library itself**. Unless you consider yourself an **advanced user** with a specific **requirement to build from source** you should [obtain a pre-made visual studio template containing the latest D++ build (for 32 and 64 bit, release and debug profiles) by clicking here](https://github.com/brainboxdotcc/windows-bot-template/) and completely skip this guide! Instead, read \ref build-a-discord-bot-windows-visual-studio.

## If you are absolutely sure you need this guide, read on:

1. Make sure you have Visual Studio 2019 or Visual Studio 2022. The Community, Professional or Enterprise versions all work, however you will probably want to install Community. You do **NOT** want to use *Visual Studio Code* for this. You can [download the correct version here](https://visualstudio.microsoft.com/downloads/).
2. Check out the DPP project source using git
3. From within Visual Studio 2019, click the "File" menu, choose "Open" then "CMake", and select the CMakeLists.txt within the project folder
   \image html winbuild_1.png
   \image html winbuild_2.png
4. Go to the "Build" menu and choose "Build all" or just press F7
   \image html winbuild_3.png 
5. Check that compilation succeeded. You may now use the library in your projects!
   \image html winbuild_4.png

## Troubleshooting

* If you do not have an option to open the CMakeLists.txt, ensure that you have installed the C++ development portions of Visual Studio (not just web development portions) with at least the default options.
* If the project does not build, please ask for help on the [official discord server](https://discord.gg/dpp).

## After compiling

After compilation you can directly reference the compiled project in your own CMakeLists.txt as a library or use the `lib/dll/headers` as you wish. Note that `openssl` and `zlib` will also be an indirect dependency of your program (as `DLL` files) and should be copied alongside `dpp.dll`.

