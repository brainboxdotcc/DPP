# Creating a Discord Bot

If you are wanting to build a bot using C++, you're in the right place! The fast and easy tutorials below will guide you through how to build a bot using the D++ library on either a UNIX-like (e.g. Linux) system with CMake or with Windows using Visual Studio 2019.

Click on a link below for a guide specifically for your system:

* \subpage creating-a-bot-application "Creating a Bot Token"
* \subpage build-a-discord-bot-windows-visual-studio "Building a discord bot in Windows using Visual Studio"
* \subpage build-a-discord-bot-windows-wsl "Building a discord bot in Windows using WSL (Windows Subsystem for Linux)"
* \subpage build-a-discord-bot-linux-clion "Building a discord bot in Linux using CLion"
* \subpage buildcmake "Building a Discord Bot using CMake/UNIX"
* \subpage building-a-cpp-discord-bot-in-repl "Creating a Discord bot in Repl.it"

\page buildcmake Building a Discord Bot using CMake/UNIX
# Building with CMake

## 1. Toolchain
Before compiling, you will need to install `cmake` on your system.
To be sure that `cmake` is installed, you can type the following command:

    $ cmake --version
    cmake version 3.20.4


## 2. Create a CMake project

In an empty directory, create the following files and directories:

    - your_project/
        |-- libs/
        |-- src/
            |-- main.cpp
        |-- CMakeLists.txt


In the `libs/` directory, clone D++ with: `git clone https://github.com/brainboxdotcc/DPP.git`

## 3. Configure CMake

Here is an example CMake configuration, adapt it according to your needs:

~~~~~~~~~~~~~~{.cmake}
# minimum CMake version required
cmake_minimum_required(VERSION 3.15)
# Project name, version and description
project(discord-bot VERSION 1.0 DESCRIPTION "A discord bot")

# Add DPP as dependency
add_subdirectory(libs/DPP)
# You can also add any other libs you want to use

# Create an executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    # your other files...
)

# Linking libraries
target_link_libraries(${PROJECT_NAME}
    dpp
    # Add any other libs you want to use here
)

# Specify includes
target_include_directories(${PROJECT_NAME} PRIVATE
    libs/DPP/include
    # Remember to add the include directories of any other libraries too
)

# Set C++ version
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
~~~~~~~~~~~~~~

Your project directory should look like this:

    - your_project/
        |-- libs/
            |-- DPP
        |-- src/
            |-- main.cpp
        |-- CMakeLists.txt


**Have fun!**

\page build-a-discord-bot-windows-visual-studio Building a discord bot in Windows using Visual Studio

To create a basic bot using **Visual Studio 2019** or **Visual Studio 2022**, follow the steps below to create a *working skeleton project you can build upon*.

1. Make sure you have Visual Studio 2019 or 2022. Community, Professional or Enterprise work fine. These instructions are not for Visual Studio Code. You can [download the correct version here](https://visualstudio.microsoft.com/downloads/). Note that older versions of Visual Studio will not work as they do not support enough of the C++17 standard.
2. Clone the [template project](https://github.com/brainboxdotcc/windows-bot-template/). Be sure to clone the entire project and not just copy and paste the cpp file.
3. Double click on the MyBot.sln file in the folder you just cloned
   \image html vsproj_1.png
4. Add your bot token (see \ref creating-a-bot-application) and guild ID to the example program 
   \image html vsproj_2.png
5. Click "Local windows debugger" to compile and run your bot!
   \image html vsproj_3.png
6.  Observe the build output. There may be warnings, but so long as the build output ends with "1 succeeded" then the process has worked. You may now run your bot!
    \image html vsproj_14.png

## Troubleshooting

- If you get an error that a dll is missing (e.g. `dpp.dll` or `opus.dll`) when starting your bot, then simply copy all dlls from the **bin** directory of where you extracted the DPP zip file to, into the same directory where your bot's executable is. You only need to do this once. There should be several of these dll files: `dpp.dll`, `zlib.dll`, `openssl.dll` and `libcrypto.dll` (or similarly named SSL related files), `libsodium.dll` and `opus.dll`. Note the template project does this for you, so you should never encounter this issue.
- Stuck? You can find us on the [official discord server](https://discord.gg/dpp) - ask away! We don't bite!

\page build-a-discord-bot-windows-wsl Building a discord bot in Windows using WSL (Windows Subsystem for Linux)

This tutorial teaches you how to create a lightweight environment for D++-development using **WSL** and **Visual Studio Code**

This Tutorial will use WSL's default distribution, **Ubuntu**! You might use other Distros if you prefer, but keep in mind the setup process might be different!

1. Make sure you have installed your WSL 2 environment properly using [this guide to setup up WSL](https://docs.microsoft.com/en-us/windows/wsl/install) and [this guide to connect to Visual Studio Code](https://docs.microsoft.com/en-us/windows/wsl/tutorials/wsl-vscode).
2. Now open PowerShell as an Admin and type `wsl` to start up your subsystem. If you want to set up a CMake project (recommended for production bots) now, consider continuing your path of becoming the master of all Discord bots [here](https://dpp.dev/buildcmake.html), otherwise keep following this guide!
3. Go to your home directory using `cd ~`
4. Download the latest build for your Distro using `wget [url here]`. In this guide we will use the latest build for 64 bit Ubuntu: `wget -O libdpp.deb https://dl.dpp.dev/latest`
5. Finally install all required deps and the library using `sudo apt-get install libopus0 && sudo apt-get install -y libopus-dev && sudo apt-get install -y libsodium-dev && sudo dpkg -i libdpp.deb && rm libdpp.deb`
6. Congratulations, you've successfully installed all dependencies! Now comes the real fun: Setting up the environment! For this tutorial we'll use a as small as possible setup, so you might create a more advanced one for production bots.
7. Navigate to a folder of your choice using `cd your/path/here` or create a new directory using `mkdir MyBot && cd MyBot`
8. Now that you've a folder to work in type `> mybot.cxx` to create a file you can work in!
9. Now you can open this file in Visual Studio Code by pressing `CTRL+SHIFT+P` and typing `Remote-WSL: New WSL Window`. This will bring up a new window. In the new window, choose `open folder` and choose the folder you've created prior. Press OK and now you have your Folder opened as a Workspace!
10. Add code to your CXX file and compile it by running `g++ -std=c++17 *.cxx -o bot -ldpp` in the same folder as your cxx file.
11. start your bot by typing `./bot`!

\page build-a-discord-bot-linux-clion Building a discord bot in Linux using CLion

This tutorial teaches you how to create a _working skeleton project you can build upon_, using the JetBrains-IDE **[CLion](https://www.jetbrains.com/clion/)**.

\note This tutorial will use **Ubuntu**! You might use other Distros if you prefer, but keep in mind the setup process might be different!

Make sure you have CLion installed and works fine (run a _hello-world program_). You can [download CLion here](https://www.jetbrains.com/de-de/clion/download/).

## Setup a project

Create a new project. Select C++17 as the Language standard, or C++20 if you want something more recent.

We'll use the following file structure as a _skeleton project you can build upon_:

    - your_project/
        |-- libs/
        |-- src/
            |-- main.cpp
        |-- CMakeLists.txt


Create the directories in your project and move the by CLion generated _hello-world main.cpp_ in the `src/` directory.

In the `libs/` directory, clone D++ with: `git clone https://github.com/brainboxdotcc/DPP.git`. You can also clone [spdlog](https://github.com/gabime/spdlog) into it if you need a logger.

Your project directory should look like this:

\image html build-clion-project-structure.png

### Configure CMake file

Paste this CMake configuration in the `CMakeLists.txt` and adapt it according to your needs:

~~~~~~~~~~~~~~{.cmake}
# minimum CMake version required
cmake_minimum_required(VERSION 3.15)
# Project name, version and description
project(discord-bot VERSION 1.0 DESCRIPTION "A discord bot")

# Add DPP as dependency
add_subdirectory(libs/DPP)
add_subdirectory(libs/spdlog) # if you need a logger. Don't forget to clone sources
                              # in the `libs/` directory

# Create an executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    # your others files...
)

# Linking libraries
target_link_libraries(${PROJECT_NAME}
    dpp
    spdlog # Like before, if you need spdlog
)

# Specify includes
target_include_directories(${PROJECT_NAME} PRIVATE
    libs/DPP/include
    libs/spdlog/include # Like before, if you need spdlog
)

# Set C++ version
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17 # or 20 if you want something more recent
    CXX_STANDARD_REQUIRED ON
)
~~~~~~~~~~~~~~

Then open the "File" menu and click on "Reload CMake Project" to reload the CMake configuration.

\image html build-clion-reload-cmake-project.png

### Add an example program

The next step is to write the bot. Copy and paste the following [example program](https://dpp.dev/firstbot.html) in the `main.cpp` and set your bot token (see [Creating a Bot Token](https://dpp.dev/creating-a-bot-application.html)) and guild ID to the example program:


~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

const std::string    BOT_TOKEN    = "add your token here";

int main() {
    dpp::cluster bot(BOT_TOKEN);

    bot.on_log(dpp::utility::cout_logger());

    bot.on_slashcommand([](const dpp::slashcommand_t& event) {
         if (event.command.get_command_name() == "ping") {
            event.reply("Pong!");
        }
    });

    bot.on_ready([&bot](const dpp::ready_t& event) {
        if (dpp::run_once<struct register_bot_commands>()) {
            bot.global_command_create(
                dpp::slashcommand("ping", "Ping pong!", bot.me.id)
            );
        }
    });

    bot.start(false);
}
~~~~~~~~~~~~~~~


Hit the green "Run" button in the top-right to run the bot.

**Congratulations, you've successfully set up a bot!**

## Troubleshooting

- Stuck? You can find us on the [official discord server](https://discord.gg/dpp) - ask away! We don't bite!

\page creating-a-bot-application Creating a Bot Token

Before you start coding, you need to create and register your bot in the Discord developer portal. You can then add this bot to your Discord-server.

## Creating a new bot

To create a new application, take the steps as follows:

1. Sign in to the [Discord developer portal](https://discord.com/developers/applications) and click on "New Application" on the top right.
2. Next, enter a name for the application in the pop-up and press the "Create" button.
\image html create_application_confirm_popup.png
In this example we named it "D++ Test Bot".
3. Move on by click the "Bot" tab in the left-hand side of the screen. Now click the "Add Bot" button on the right and confirm that you want to add the bot to your application.

\image html create_application_add_bot.png

On the resulting screen, you’ll note a page with information regarding your new bot. You can edit your bot name, description, and avatar here if you want to. If you wish to read the message content from messages, you need to enable the message content intent in the "Privileged Gateway Intents" section.

\image html create_application_bot_overview.png

In this panel, you can get your bot token by clicking "Reset Token". A bot token looks like this: `OTAyOTMxODU1NTU1MzE3ODUw.YXlm0g.9oYCt-XHXVH_z9qAytzmVRzKWTg`

\warning **Do not share this token** with anybody! If you ever somehow compromise your current bot token or see your bot in danger, you can regenerate the token in the panel.

## Adding the bot to your server

Once you've created your bot in the discord developer portal, you may wonder:
> Where is my bot now, I can't see him on my server?!

That's because you've created a bot application, but it's not on any server right now. So, to invite the bot to your server, you must create an invitation URL.

1. go again into the [Applications page](https://discord.com/developers/applications) and click on your bot.
2. Go to the "OAuth2" tab and click on the subpage "URL Generator".
\image html create_application_navigate_to_url_generator.png
3. Select the `bot` scope. If your bot uses slash commands, also select `applications.commands`. You can read more about scopes and which you need for your application [here](https://discord.com/developers/docs/topics/oauth2#shared-resources-oauth2-scopes).
4. Choose the permissions required for your bot to function in the "Bot Permissions" section.
5. Copy and paste the resulting URL in your browser. Choose a server to invite the bot to, and click "Authorize".


\note For bots with elevated permissions, Discord enforces two-factor authentication on the bot owner's account when added to servers that have server-wide 2FA enabled.

## Troubleshooting

- Stuck? You can find us on the [official discord server](https://discord.gg/dpp) - ask away! We don't bite!

\page building-a-cpp-discord-bot-in-repl Creating a Discord bot in Repl.it

@note There is a premade repl, ready for use which was built using the steps above. If you wish to use this repl simply [visit this github repository](https://github.com/alanlichen/dpp-on-repl) and click the "Run on Replit" button. Then, follow the steps in the README file.

To build a D++ bot in a repl.it instance, follow these steps. These steps are slightly more convoluted than installing D++ into a standard container as we don't have access to root in the conventional way or write access to any files outside of our home directory in a repl. This guide sidesteps the issue by locally extracting a libdpp deb file installer, and referencing the local dependencies from the command-line.

1. Use wget, or the upload button, to get the precompiled x64 release into your repl as a file, e.g. `wget -O libdpp.deb https://dl.dpp.dev/latest`
2. Extract this deb file using `dpkg`:
```
dpkg -x libdpp.deb .
```
3. Compile your bot, note that you should be sure to include the `pthread` library explicitly and reference the extracted dpp installation you just put into the repl:
```
g++ -o bot main.cpp -ldpp -lpthread -L./usr/lib -I./usr/include -std=c++17
```
4. Run your bot! Note that you will need to set `LD_PRELOAD` to reference `libdpp.so` as it will be located in `$HOME` and not `/usr/lib`:
```
LD_PRELOAD=./usr/lib/libdpp.so ./bot
```

Now that your bot is running, you have to keep it online. Replit automatically puts repls to sleep after some time, so you will need to ping a webserver. Unfortunately, Replit is sometimes limiting, and this is one of the only free workarounds to this issue.

1. Start a http server. This can be through any webserver, but as a simple solution, use python's built in http.server:
```
python3 -m http.server
```
2. Create an index.html file with anything inside it for the server to serve.
3. Go to [uptimerobot.com](https://uptimerobot.com/) and create an account if you dont have one.
4. After verifying your account, click "Add New Monitor".
+ For Monitor Type, select "HTTP(s)"
+ In Friendly Name, put the name of your bot
+ For your url, copy the url of the new website that repl is serving for you
+ Select any alert contacts you want, then click "Create Monitor"
Here is an example of a possible uptimerobot configuration:

\image html uptimerobot.png

## Troubleshooting

If the bot fails to start and instead you receive an error message about being banned from the Discord API, there is little to be done about this. These bans are temporary but because repl.it is a shared platform, you share an IP address with many thousands of bots, some abusive and some badly written. This will happen often and is outside of the control of yourself and us. However, you can try to migitate this by typing `kill 1` in the shell. This is not guaranteed to work, and you might need to try it a few times. If it still does not work, then we recommend instead you obtain some affordable non-free hosting instead.

If your bot continues to fall asleep even though you have a server, we advise you to double check that no errors are happening, and if the server is being pinged. If that still does not work, we again recommend you to obtain some affordable non-free hosting.
