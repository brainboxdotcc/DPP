\page install-vcpkg Installing from VCPKG (Windows, Linux, OSX)

To install D++ on a system from VCPKG:

- Ensure VCPKG is correctly installed, and run `vcpkg integrate install` to integrate it with your preferred IDE. This has been reported to work with Visual Studio, vscode, and JetBrains CLion.
- From a command line, type `vcpkg install dpp:x64-windows` (replace `x64-windows` with whichever OS and architecture you want the library to be built for)
\image html vcpkg.png
- VCPKG will install the library and dependencies for you! Once completed you will receive a message indicating success:

- Use `vcpkg list dpp` to check that the package is installed:
```
c:\vcpkg>vcpkg list dpp
dpp:x64-windows                                    10.0.15          D++ Extremely Lightweight C++ Discord Library.
```
- You may now use the library within a `CMake` based project by adding instructions such as these to your `CMakeLists.txt`:
```cmake
    find_package(dpp CONFIG REQUIRED)
    target_link_libraries(your_target_name PRIVATE dpp::dpp)
```
