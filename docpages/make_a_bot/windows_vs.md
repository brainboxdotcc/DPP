\page build-a-discord-bot-windows-visual-studio Building a Discord Bot on Windows Using Visual Studio

To create a basic bot using **Visual Studio 2019** or **Visual Studio 2022**, follow the steps below to create a *working skeleton project you can build upon*.

If you prefer a video tutorial, you can watch the video below! Otherwise, scroll past and keep reading!

## Video Tutorial

\htmlonly

<iframe width="560" height="315" src="https://www.youtube.com/embed/JGqaQ9nH5sk?si=dung8KuYbWvP2_oL" title="YouTube video player" frameborder="0" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share" allowfullscreen></iframe>

\endhtmlonly

## Text Tutorial

1. Make sure you have Visual Studio 2019 or 2022. Community, Professional or Enterprise work fine. These instructions are not for Visual Studio Code. You can [download the correct version here](https://visualstudio.microsoft.com/downloads/). Note that older versions of Visual Studio will not work as they do not support enough of the C++17 standard.
2. Clone the [template project](https://github.com/brainboxdotcc/windows-bot-template/). Be sure to clone the entire project and not just copy and paste the `.cpp` file.
3. Double click on the `MyBot.sln` file in the folder you just cloned:
\image html vsproj_1.png
4. Add your bot token (see \ref creating-a-bot-application) and guild ID to the example program:
\image html vsproj_2.png
5. Click "Local Windows debugger" to compile and run your bot!
\image html vsproj_3.png
6. Observe the build output, so long as the build output ends with "1 succeeded" then the process has worked. You may now run your bot!
\image html vsproj_14.png

## Troubleshooting

- If you get an error that looks like this: \code{.unparsed} 1>MyBot.obj : error LNK2019: unresolved external symbol "__declspec(dllimport) public: class dpp::async<struct dpp::confirmation_callback_t>
__cdecl dpp::interaction_create_t::co_reply(class std::basic_string<char,struct std::char_traits<char>,class std::allocator<char> > const &)const
" (__imp_?co_reply@interaction_create_t@dpp@@QEBA?AV?$async@Uconfirmation_callback_t@dpp@@@2@AEBV?$basic_string@DU?$char_traits@D@std@@V?$allocator@D@2@@std@@@Z)
referenced in function "public: class dpp::task<void> __cdecl `int __cdecl main(void)'::`2'::<lambda_2>$_ResumeCoro$1::operator()(struct dpp::slashcommand_t const &)const "
(??R<lambda_2>$_ResumeCoro$1@?1??main@@YAHXZ@QEBA?AV?$task@X@dpp@@AEBUslashcommand_t@3@@Z)
1>...\windows-bot-template-main\x64\Debug\MyBot.exe : fatal error LNK1120: 1 unresolved externals
1>Done building project "MyBot.vcxproj" -- FAILED.
\endcode Make sure your don't have another version of the library installed through vcpkg. The template uses a slightly different version of D++ that has coroutines, while the vcpkg version does not, and the latter overwrites it! Uninstalling the library through vcpkg should fix this issue.
- If you get an error that a DLL is missing (e.g. `dpp.dll` or `opus.dll`) when starting your bot, then simply copy all DLLs from the **bin** directory of where you cloned the D++ repository to, into the same directory where your bot's executable is. You only need to do this once. There should be several of these DLL files: `dpp.dll`, `zlib.dll`, `openssl.dll` and `libcrypto.dll` (or similarly named SSL related files), `libsodium.dll` and `opus.dll`. Note the template project does this for you, so you should never encounter this issue.
- If you get an error that says "Debug/Release mismatch", **you are using the wrong configuration of the D++ dll**. Your bot's executable and the dpp.dll file should both be built in the same configuration (Release or Debug), you get this error if they are different. **This also means you altered the template in a significant way,** we recommend you undo your modifications or reinstall the template.
- Stuck? You can find us on the [official Discord server](https://discord.gg/dpp) - ask away! We don't bite!
