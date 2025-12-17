\page build-a-discord-bot-linux-clion Building a Discord Bot using CLion (Linux)

\warning **This tutorial assumes you are using Ubuntu**. You might use other distros if you prefer, but keep in mind the setup process might be different! This tutorial also teaches you how to use DPP with CMake, using the JetBrains IDE **[CLion](https://www.jetbrains.com/clion/)**. If you have not installed CLion, You can [download CLion here](https://www.jetbrains.com/de-de/clion/download/). If you do not have DPP installed, visit \ref buildcmake "this page" on how to setup the project using a precompiled version of DPP. If you want to use source and haven't set that up, look towards \ref buildlinux "this page" on how to do so. **This tutorial will not teach you how to setup CMake and will assume you have already done so**.

### Add an example program

Open up CLion and open the folder for your bot. You may notice that CLion will start doing the whole CMake process and it will create a folder called `cmake-build-debug`, this is normal so don't be alarmed! It is just CLion registering all the CMake stuff so it can build and give you auto-suggestions.

Now, you can open your `main.cpp` file. If you have code there, then you're one step ahead! If not, copy and paste the following \ref firstbot "example program" in the `main.cpp` and set your bot token (see \ref creating-a-bot-application). Here's how your `main.cpp` file should look:

\include{cpp} firstbot.cpp

Now, you can go ahead and hit the green "Run" button in the top-right to run the bot.

**Congratulations, you've successfully set up a bot!**

If you're stuck, come find us on the [official discord server](https://discord.gg/dpp)! Ask away! We don't bite!
