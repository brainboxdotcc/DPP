\page install-windows-vs-zip Installing from zip (Windows) for Visual Studio

To add D++ to a Visual Studio project, using **Visual Studio 2019** or **Visual Studio 2022**, follow the steps below. The steps below assume an empty project, if you are adding to an existing project simply skip steps 1 through 4, and step 13.

\note It is possible to skip this entire tutorial, and obtain a \ref build-a-discord-bot-windows-visual-studio "pre-made Visual Studio template containing the latest D++ build (for 32 and 64 bit, release and debug profiles) by clicking here".

1. Make sure you have Visual Studio 2019 or 2022. Community, Professional or Enterprise work fine. These instructions are not for Visual Studio Code. You can [download the correct version here](https://visualstudio.microsoft.com/downloads/). Note that older versions of Visual Studio will not work as they do not support enough of the C++17 standard.

2. Start Visual Studio and choose to create a new project
\image html zip_vsproj_1.png

3. Choose the project type "Console Project" and click next
\image html zip_vsproj_2.png

4. Name your bot project. In this example, we'll use the name 'MyBot'. You can have any name you like here.
\image html zip_vsproj_3.png

5. Open the zip file you downloaded which contains the D++ DLLs, include files, and lib file. Drag and drop this to your local machine and make note of where you placed it. This location will be important in later steps.
\image html zip_vsproj_4.png

6. Back inside Visual Studio, right click on the project (not solution!) in the tree in your Visual Studio window. choose 'Properties'.
\image html zip_vsproj_5.png

7. The next step is to populate the include directories and library directories sections with the paths to the D++ library and include files. The next steps will guide you through how to do this.
\image html zip_vsproj_6.png

8. Click 'edit' when prompted to edit the include paths section. Add the path to the include folder you extracted to your machine, which we took note of earlier. Note that it is important to add the dpp-10.0 folder, not any other folder, to this list:
\image html zip_vsproj_7.png

9.  Going back to the previous window, now edit the library paths. Again click 'edit' when prompted to edit the library paths section. Add the path to the library folder you extracted to your machine, which we took note of earlier. Note that once more it is important to add the dpp-10.0 folder within it, not any other folder, to this list. Also be aware this is a **different folder** than the one you just added for includes!
\image html zip_vsproj_8.png

10. Double check at this point that all the directories are filled in correctly. They should look generally like the ones in the screenshot below:
\image html zip_vsproj_9.png

11. Go to the general section in the same window now, and look for the drop down list labelled "C++ Language Standard". Make sure the selected option is **C++17 Standard (/std:c++17)** or later.
\image html zip_vsproj_10.png

12. Again within the same window, go to the input section, under the linker category, and add '**dpp.lib;**' to the start of the libraries to include, as shown below:
\image html zip_vsproj_11.png

13. Now you can paste some code into the editor, completely replacing the 'hello world' application that Visual Studio made for you. The example code here is the basic bot from the first example on this site. You should at this point also double check that the architecture you have selected (in this case x86) matches the version of the dll/lib files you downloaded from the website. This is **important** as if you mismatch them the compilation will just fail.
\image html zip_vsproj_12.png

14. Go to the build menu and choose Build Solution (A handy shortcut for this is to just press **F7**):
\image html zip_vsproj_13.png

15. Observe the build output. There may be warnings, but so long as the build output ends with "1 succeeded" then the process has worked. You may now run your bot!
\image html zip_vsproj_14.png

## Troubleshooting

\note A much easier way of getting a bot running is available \ref build-a-discord-bot-windows-visual-studio "here"! We recommend using that one if this process seems too complex or you run into too many issues.

- If you get an error that a DLL is missing (e.g. `dpp.dll` or `opus.dll`) when starting your bot, then simply copy all DLLs from the **bin** directory of where you extracted the D++ zip file to, into the same directory where your bot's executable is. You only need to do this once. There should be several of these DLL files: `dpp.dll`, `zlib.dll`, `openssl.dll` and `libcrypto.dll` (or similarly named SSL related files), `libsodium.dll` and `opus.dll`.
- Please note that if you change the architecture (step 13) you must reconfigure all of steps 7 through 12 again as these configurations are specific to each architecture. This is to allow for different sets of precompiled libs, e.g. for `x86`, `x64`, etc.
- If you get an error that says "Debug/Release mismatch", **you are using the wrong configuration of the D++ dll**. Your bot's executable and the dpp.dll file should both be built in either Release or Debug, you get this error if they are different. We recommend using \ref build-a-discord-bot-windows-visual-studio "the bot template", it has all those things already set up.
- You should run your bot from a command prompt. If you do not, and it exits, you will not be able to see any output as the window will immediately close.
- If you need to update the `opus.dll` or `zlib.dll` (or any other prebuilt dll) these can be obtained by requesting them to be installed via `vcpkg` then copying the dlls, libraries and headers from the vcpkg `install` folder.
- Stuck? You can find us on the [official discord server](https://discord.gg/dpp) - ask away! We don't bite!

