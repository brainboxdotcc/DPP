\page build-a-discord-bot-windows-wsl Building a Discord Bot on Windows Using WSL (Windows Subsystem for Linux)

This tutorial teaches you how to create a lightweight environment for D++ development using **WSL** and **Visual Studio Code**

\note **This Tutorial will use WSL's default distribution, Ubuntu**! You can use other distros if you wish, **but keep in mind the setup process might be different!** If you're aiming for production, we recommend you continue your path of becoming the master of all Discord bots \ref buildcmake "by visiting this page", otherwise keep following this guide!

1. Make sure you have installed your WSL 2 environment properly using [this guide to setup up WSL](https://docs.microsoft.com/en-us/windows/wsl/install) and [this guide to connect to Visual Studio Code](https://docs.microsoft.com/en-us/windows/wsl/tutorials/wsl-vscode).
2. Now open PowerShell as Administrator and type `wsl` to start up your subsystem. You may also type `ubuntu` into your search bar and open it that way.
3. Head on over to your home directory using `cd ~`.
4. Download the latest build for your distro using `wget [url here]`. In this guide we will use the latest build for 64 bit Ubuntu: `wget -O libdpp.deb https://dl.dpp.dev/latest`.
5. Finally install all required dependencies and the library itself using `sudo apt-get install libopus0 libopus-dev libsodium-dev && sudo dpkg -i libdpp.deb && rm libdpp.deb`.
6. Congratulations, you've successfully installed all dependencies! Now comes the real fun: Setting up the environment! For this tutorial we'll use a as small as possible setup, so you might create a more advanced one for production bots.
7. Create a new directory, inside your home directory, using `mkdir MyBot`. Then, you want to open that directory using `cd MyBot`.
8. Now that you've a directory to work in, type `touch mybot.cxx` to create a file you can work in!
9. Now, head on over to Visual Studio Code. Press `CTRL+SHIFT+P` and type `Remote-WSL: New WSL Window` (You don't have to type all of it, it will auto-suggest it!). This will bring up a new window. In the new window, choose `open folder` and choose the directory you've created prior (It should be within your home directory). Press OK and now you have your Folder opened as a Workspace!
10. Add code to your CXX file (We suggest using the \ref firstbot "first bot page" if this is your first time!) and compile it by running `g++ -std=c++17 *.cxx -o bot -ldpp` in the same folder as your cxx file. This will create a "bot" file!
11. You can now start your bot by typing `./bot`!

If everything was done right, you should be able to see your bot working!
