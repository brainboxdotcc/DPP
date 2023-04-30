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

