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

    bot.start(dpp::st_wait);
}
~~~~~~~~~~~~~~~


Hit the green "Run" button in the top-right to run the bot.

**Congratulations, you've successfully set up a bot!**

## Troubleshooting

- Stuck? You can find us on the [official discord server](https://discord.gg/dpp) - ask away! We don't bite!

