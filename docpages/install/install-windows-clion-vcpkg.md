\page install-windows-clion-vcpkg Installing D++ for CLion via VCPKG (Windows)

\warning This page is for **Windows only**. If you want to use CLion on Linux, look to use \ref build-a-discord-bot-linux-clion "this page". Like always with windows, we highly recommends you use the [pre-made Visual Studio template](https://github.com/brainboxdotcc/windows-bot-template/)

## Changing Toolchains

\note If you have already configured your toolchain to use anything **but** MinGW64, then you can skip to the next section. Otherwise, it is **critical** that you follow along with this section.

Head on over to `File > Settings` (Ctrl+Alt+S), then navigate to `Build, Execution, Deployment > Toolchains`.

If there is a Visual Studio toolchain there, drag it to the top of the list. This will make the Visual Studio toolchain the default toolchain.

\image html clionvstoolchain.png

If you don't have the Visual Studio toolchain, you can hit the plus symbol above the list of toolchains and add a toolchain. This is also how you can add WSL as a toolchain!

\image html clionvstoolchain2.png

From there, you need to drag it to the top (if it didn't already add at the top) to ensure it's the default toolchain.

## VCPKG setup

1. Build [VCPKG](https://vcpkg.io/) on your system (skip if you already have it).
2. Run `vcpkg install dpp:x64-windows`.
3. VCPKG will install the library along with the dependencies for you. Note, this can take a while and may eat up your system's resources.
4. Now, check if D++ has been installed using `vcpkg list dpp`.
```cmd
C:/vcpkg>vcpkg list dpp
dpp:x64-windows     10.0.29     D++ Extremely Lightweight C++ Discord Library.
```

## Using VCPKG with CLion 

1. To use vcpkg in CLion, add the following line to your CMake options in the settings (Located under `Settings > Build, Execution, Deployment > CMake`)
```cmd
-DCMAKE_TOOLCHAIN_FILE=path_to_vcpkg_root_folder/scripts/buildsystems/vcpkg.cmake
```
   For example, if your root folder is `C:/vcpkg/` then the CMake option will be:
```cmd
-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

2. Now proceed to add the following lines to your `CMakeLists.txt`:
```cmake
find_package(dpp CONFIG REQUIRED)
target_link_libraries(main PRIVATE dpp::dpp)
target_include_directories(main PRIVATE path_to_vcpkg_root_folder/installed/architecture-os/include)
```
   For example, if your VCPKG root is `C:/vcpkg/`, then your `CMakeLists.txt` should look like this:
```cmake
find_package(dpp CONFIG REQUIRED)
target_link_libraries(main PRIVATE dpp::dpp)
target_include_directories(main PRIVATE C:/vcpkg/installed/x64-windows/include)
```
   
3. Congratulations! Now try to build a test program to see if the installation succeeded.

If you get stuck somewhere, feel free to ask us on the D++ [discord server](https://discord.gg/dpp).

## Troubleshooting

If you see a message like `Detecting C compiler ABI info - failed` or something along the lines of `"cl.exe" is not able to compile a simple test program`, then try to reinstall `Windows Build Tools` along with the `Windows SDK` from the Visual Studio Installer. 
