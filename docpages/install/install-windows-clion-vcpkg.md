\page install-windows-clion-vcpkg Installing D++ for CLion via VCPKG (Windows)

\warning This page is for **Windows only**. If you want to use CLion on Linux, look to use \ref build-a-discord-bot-linux-clion "this page". Like always with windows, we highly recommends you use the [pre-made Visual Studio template](https://github.com/brainboxdotcc/windows-bot-template/)

To add D++ to a CLion project, you need obtain the library through VCPKG and then configure your CLion project and `CMakeLists.txt`.

1. Build [VCPKG](https://vcpkg.io/) on your system (skip if you already have it).
2. Run `vcpkg install dpp:x64-windows`.
3. VCPKG will install the library along with the dependencies for you.
4. Now check if dpp has been installed using `vcpkg list dpp`.
```cmd
C:/vcpkg>vcpkg list dpp
dpp:x64-windows     10.0.23     D++ Extremely Lightweight C++ Discord Library.
```
   
5. To use vcpkg in CLion, add the following line to your CMake options in the settings (Located under Settings > Build, Execution, Deployment > CMake)
```cmd
-DCMAKE_TOOLCHAIN_FILE=path_to_vcpkg_root_folder/scripts/buildsystems/vcpkg.cmake
```
   For example, if your root folder is `C:/vcpkg/` then the CMake option will be:
```cmd
-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

6. Now proceed to add the following lines to your `CMakeLists.txt`:
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
   
7. Congratulations! Now try to build a test program to see if the installation succeeded. If you get stuck somewhere, feel free to ask us on the D++ [discord server](https://discord.gg/dpp).