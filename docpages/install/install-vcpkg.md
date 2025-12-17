\page install-vcpkg Installing from VCPKG (Windows)

\warning **We do not support VCPKG for any platform that isn't Windows. This does not mean VCPKG doesn't work, it just means we do not test it.** If you are using other platforms then please look towards our other pages. We also advise that you use the [pre-made Visual Studio template](https://github.com/brainboxdotcc/windows-bot-template/) on Windows, as VCPKG takes longer to receive updates.

To install D++ on a system with VCPKG:

- Ensure VCPKG is correctly installed, and run `vcpkg integrate install` to integrate it with your preferred IDE. This has been reported to work with Visual Studio, VSCode, and JetBrains CLion.
- From a command line, type `vcpkg install dpp:x64-windows`

\image html vcpkg.png

- VCPKG will install the library, and dependencies, for you! Once completed, you will receive a message, indicating that dpp successfully installed!
- Use `vcpkg list dpp` to check that the package is installed, as so:

```cmd
c:\vcpkg>vcpkg list dpp
dpp:x64-windows     10.0.29     D++ Extremely Lightweight C++ Discord Library.
```

- You may now use the library within a `CMake` based project by adding the following instructions to your `CMakeLists.txt`:

```cmake
find_package(dpp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE dpp::dpp)
```
