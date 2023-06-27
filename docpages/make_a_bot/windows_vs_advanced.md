\page build-a-discord-bot-windows-advanced Building a discord bot in Windows Visual Studio (avanced users only)

\warning Please be aware that this article is written for **advanced users** that already have experience using C++! **Don't be ashamed** to use the [template project](https://github.com/brainboxdotcc/windows-bot-template). As this is considered advanced content, you may check out \ref build-a-discord-bot-windows-visual-studio. We don't offer any support for manual setups, please use the template if you can't get it running.
  
  
Open Visual Studio 2022 and create a new project. Choose `Console App` as project type. You can filter for `C++`, `Windows`, `Console` to find it.
  
  
In this article we are going to call the project `DiscordBotto`. Keep all the default settings and generate the project.
Remember where your project is located at. In my case the path is: `D:\Programming\DiscordBotto`. This is the so called "SolutionDir" - keep that in mind!

Go to your Solution Directory and create two folders: `deps` (for dependencies) and inside of `deps` we want a `dpp` directory.
It should look like this:
```
│   DiscordBotto.sln
│
├───deps
│   └───dpp
│
└───DiscordBotto
        DiscordBotto.cpp
        DiscordBotto.vcxproj
        DiscordBotto.vcxproj.filters
        DiscordBotto.vcxproj.user
```

Inside of `dpp` we can create two more directories for our build setups: `debug` for Debug-Mode and `release` for Release-Mode.

```
│   DiscordBotto.sln
│
├───deps
│   └───dpp
│       ├───debug
│       └───release
│
└───DiscordBotto
        DiscordBotto.cpp
        DiscordBotto.vcxproj
        DiscordBotto.vcxproj.filters
        DiscordBotto.vcxproj.user
```

For the next part we have to grab the latest release archives from the release page. You can do that by visiting [the release page](https://github.com/brainboxdotcc/DPP/releases/latest).
Scroll down to assets and download the correct archives for your setup. As my Windows installation is running on a x64 machine, I'll use win64 archives.
At the time of writing this (release v10.0.24) the correct files are `libdpp-10.0.24-win64-debug-vs2022.zip` and `libdpp-10.0.24-win64-release-vs2022.zip`.

Open the archives and put the `bin`, `include` and `lib` directory inside the `debug` and `release` directory - please pay attention to the zip you opened as this step is really important!
The `.zip`-file containing `release` in its name is meant for the `release` directory. The same rule applies to `debug`!

Your structure should look like this:
```
│   DiscordBotto.sln
│
├───deps
│   └───dpp
│       ├───debug
│       │   ├───bin
│       │   │       dpp.dll
│       │   │       libcrypto-1_1-x64.dll
│       │   │       libsodium.dll
│       │   │       libssl-1_1-x64.dll
│       │   │       opus.dll
│       │   │       zlib1.dll
│       │   │
│       │   ├───include
│       │   │   └───dpp-10.0
│       │   │       └───dpp
│       │   │           │   [ a ton of includes here ]
│       │   │           │
│       │   │           └───nlohmann
│       │   │                   json.hpp
│       │   │                   json_fwd.hpp
│       │   │
│       │   └───lib
│       │       ├───cmake
│       │       │   └───dpp
│       │       │           dpp-config-version.cmake
│       │       │           dpp-config.cmake
│       │       │           dpp-release.cmake
│       │       │           dpp.cmake
│       │       │
│       │       └───dpp-10.0
│       │               dpp.lib
│       │
│       └───release
│           ├───bin
│           │       dpp.dll
│           │       libcrypto-1_1-x64.dll
│           │       libsodium.dll
│           │       libssl-1_1-x64.dll
│           │       opus.dll
│           │       zlib1.dll
│           │
│           ├───include
│           │   └───dpp-10.0
│           │       └───dpp
│           │           │   [ a ton of includes here ]
│           │           │
│           │           └───nlohmann
│           │                   json.hpp
│           │                   json_fwd.hpp
│           │
│           └───lib
│               ├───cmake
│               │   └───dpp
│               │           dpp-config-version.cmake
│               │           dpp-config.cmake
│               │           dpp-release.cmake
│               │           dpp.cmake
│               │
│               └───dpp-10.0
│                       dpp.lib
│
└───DiscordBotto
        DiscordBotto.cpp
        DiscordBotto.vcxproj
        DiscordBotto.vcxproj.filters
        DiscordBotto.vcxproj.user
```

As we setup our project files it is time to configure our solution to use them.
Open your solution (the `.sln`-file inside your Solution Directory) and enable the so called "Solution Explorer". You can do that with CTRL + ALT + L or by following these steps:
`Navigation Bar -> View -> Solution Explorer`.

Inside of `Solution DiscordBotto`, you should be able to spot our `Project` with the name `DiscordBotto`. Select it with left click and right click it. Click on "Properties" on the very end of the list.

At the top of your Properties Panel you'll find an option for switching "Configurations". This is important as there are big differences between a `Release` and `Debug` build!
To make this guide a little more readable we won't configure both but use \<CONFIG\> as placeholder for `debug` and `release`! Don't forget to configure both.

**HINT:** Some of the configurations we are going to change may already contain some data. You can separate your input from the rest using `;` as separation character.
You see `a` but you need to specify `b`? The result will look like this: `a;b`.  
  
Let's get started!  
**ATTENTION:** Some directory names may change with changing release number! Please be aware of that and use common sense! Use the directory structure above to identify changes.  
  
  
**1. Setting the C++ 17 Language Standard**  
Configuration Properties -> General -> C++ Language Standard -> Go the right side, select the arrow and choose something containing `C++ 17`  
  
**2. Adding our include directory**  
Setting the include directory: Configuration Properties ->"C/C++" -> General -> Additional Include Directories -> Add `$(SolutionDir)\deps\dpp\<CONFIG>\include\dpp-10.0` to the list  
  
**3. Set the D++ Preprocessor Definitions**  
Configuration Properties ->"C/C++" -> Preprocessor -> Preprocessor Definitions -> Add `DPP_BUILD;FD_SETSIZE=1024` to the list  
  
**4. Set the Standard Conforming Preprocessor (same page)**  
Use Standard Conforming Preprocessor -> `Yes (/Zc:preprocessor)`  
  
**5. C++ Compiler Command Line option**  
"C/C++" -> Command Line -> Additional Options -> Set it to: `%(AdditionalOptions) /bigobj`  
  
**6. Linking the D++ library itself**  
Configuration Properties -> Linker -> General -> Additional Library Directories -> `$(SolutionDir)\deps\dpp\<CONFIG>\lib\dpp-10.0`  
and  
Configuration Properties -> Linker -> Input -> Additional Dependencies -> Add `dpp.lib`  
  
**7. Copy all the dependencies to your executable**  
Configuration Properties -> Build Events -> Post-Build Event -> Command Line -> Set it to `xcopy /Y "$(SolutionDir)\deps\dpp\<CONFIG>\bin\*.dll" "$(OutDir)"`  