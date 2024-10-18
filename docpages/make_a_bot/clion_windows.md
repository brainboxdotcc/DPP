\page install-windows-clion-vcpkg Building a Discord Bot using CLion & VCPKG (Windows)

\warning This page is for **Windows only**. If you want to use CLion on Linux, look to use \ref build-a-discord-bot-linux-clion "this page". Like always with windows, we highly recommends you use the [pre-made Visual Studio template](https://github.com/brainboxdotcc/windows-bot-template/). This tutorial also assumes you have installed D++ via VCPKG already. If you haven't, look at \ref install-vcpkg "this page". 

### Changing Toolchains

\note If you have already configured your toolchain to use anything **but** MinGW64, then you can skip to the next section. Otherwise, it is **critical** that you follow along with this section. It should also be noted that you **need** Visual Studio for this.

Head on over to `File > Settings` (Ctrl+Alt+S), then navigate to `Build, Execution, Deployment > Toolchains`.

If there is a Visual Studio toolchain there, drag it to the top of the list. This will make the Visual Studio toolchain the default toolchain.

\image html clionvstoolchain.png

If you don't have the Visual Studio toolchain, you can hit the plus symbol above the list of toolchains and add a toolchain. This is also how you can add WSL as a toolchain!

\image html clionvstoolchain2.png

From there, you need to drag it to the top (if it didn't already add at the top) to ensure it's the default toolchain.

### Using VCPKG with CLion

To use vcpkg in CLion, add the following line to your CMake options in the settings (Located under `Settings > Build, Execution, Deployment > CMake`)
```cmd
-DCMAKE_TOOLCHAIN_FILE=path_to_vcpkg_root_folder/scripts/buildsystems/vcpkg.cmake
```
For example, if your root folder is `C:/vcpkg/` then the CMake option will be:
```cmd
-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```
   
### Making a test application.

Open your `main.cpp` file and then copy and paste the following \ref firstbot "example program" in. Then, set your bot token (see \ref creating-a-bot-application). Here's how your `main.cpp` file should look:

\include{cpp} firstbot.cpp

If everything went well, you should now have a functioning bot! If not, feel free to ask us on the D++ [discord server](https://discord.gg/dpp).

### Troubleshooting

If you see a message like `Detecting C compiler ABI info - failed` or something along the lines of `"cl.exe" is not able to compile a simple test program`, then try to reinstall `Windows Build Tools` along with the `Windows SDK` from the Visual Studio Installer. 
