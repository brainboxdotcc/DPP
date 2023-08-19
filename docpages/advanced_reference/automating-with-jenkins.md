\page automating-with-jenkins Automating your bot with Jenkins

\note This page does NOT go into explaining how to install Jenkins, nor how to initally setup Jenkins. This is a tutorial for the CMake version with Linux (more specifically Ubuntu 22.04 LTS). If you don't know how to use CMake or you don't use CMake for your bot (and would like to) then please visit [Building a Discord Bot using CMake/Unix](/buildcmake.html). If you wish to automate this tutorial from GitHub pushes then you can simply download the GitHub plugin for Jenkins, set that up and this tutorial will still work as this tutorial will only build what it can see!

### Getting Started

First of all, you'll want to create your project. For this, we'll use a Freestyle project as we're just going to be called some bash commands to tell CMake to build. We'll be calling this "DiscordBot" but you can name is whatever you want (I would advise against non-ascii characters).

\image html jenkinsproject.png

From here, just hit "Ok" and you've created your Jenkins project, Well done! From here you can add a description, change the security policy (if your jenkins is public) and whatnot.

### Automating the Building process.

Scrolling down, you'll find "Build Steps" (You can also click "Build Steps" on the left). Here, you'll want to hit "Add build step" and hit "Execute shell".

\image html buildstepjenkins.png

Inside of this, you'll want to enter this script below.

~~~~~~~~~~
# Check if the "build" directory doesn't exist (if you've not setup CMake or deleted its content).
if [ ! -d "build/" ] 
then # As it doesn't, create the build directory.
	mkdir build
	cd build
	cmake .. # Begin the CMake initialisation.
fi

# Commence build.
cmake --build build/
~~~~~~~~~~