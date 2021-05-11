mkdir build
cd build
cmake -DCMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=TRUE -DBUILD_SHARED_LIBS=TRUE-DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=../my/project/compiler-settings-toolchain.cmake  ..
"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe" libdpp.sln
copy /y C:\vckpg\installed\x64-windows\bin\zlib1.dll Debug\zlib1.dll
copy /y C:\vckpg\installed\x64-windows\bin\libcrypto-1_1-x64.dll Debug\libcrypto-1_1-x64.dll
copy /y C:\vckpg\installed\x64-windows\bin\libssl-1_1-x64.dll Debug\libssl-1_1-x64.dll
rd /q /y package
md package
md package\bin
md package\lib
md package\include
xcopy /s/q Debug\*.dll package\bin\
xcopy /s/q Debug\*.pdb package\lib\
xcopy /s/q Debug\*.lib package\lib\
xcopy /s/q ..\include\*.* package\include\
