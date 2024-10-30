\page install-conan Installing with Conan

To install D++ into a project using conan 2.0 and cmake:

- Ensure conan is correctly installed, the most popular method is with pip.
- Create a conanfile.txt in the root of the project.

```conanfile
[requires]
dpp/10.0.34

[generators]
CMakeDeps
CMakeToolchain
```

- You may now use the library within a `CMake` based project by adding instructions such as these to your `CMakeLists.txt`:

```cmake
set(CMAKE_PREFIX_PATH  "${CMAKE_CURRENT_LIST_DIR}/build")
find_package(dpp CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} dpp::dpp)
```

- If you recieve any errors about CXX version you may need to set it

```
set(CMAKE_CXX_STANDARD 17)
```

- You will then also want to set your build type to match what you will be using in conan(Debug or Release)

```
set(CMAKE_BUILD_TYPE Debug)
```

OR

```
set(CMAKE_BUILD_TYPE Release)
```

- Now run the following commands

```
mkdir build
conan install . --output-folder=build --build=missing -s build_type=Release
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="conan_toolchain.cmake"
cmake --build build --config Release
```

- NOTE: build_type= needs to match whatever was set in your CMake or you will get linker issues.
