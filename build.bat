mkdir build
cd build
cmake -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=TRUE -DBUILD_SHARED_LIBS=TRUE-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=../my/project/compiler-settings-toolchain.cmake  ..
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" test.sln
copy /y bin\Debug\fmtd.dll Debug\fmtd.dll
copy /y C:\vckpg\installed\x64-windows\bin\zlib1.dll Debug\zlib1.dll