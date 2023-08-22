\page automating-with-jenkins Automating your bot with Jenkins

\note This page does NOT go into explaining how to install Jenkins, nor how to initially setup Jenkins. This is a tutorial for the CMake version with Linux (more specifically Ubuntu 22.04 LTS). If you don't know how to use CMake or you don't use CMake for your bot (and would like to) then please visit [Building a Discord Bot using CMake/Unix](/buildcmake.html). If you wish to automate this tutorial from GitHub pushes then you can simply download the GitHub plugin for Jenkins, set that up and this tutorial will still work as this tutorial will only build what it can see!

### Getting started

First of all, you'll want to create your project. For this, we'll use a Freestyle project as we're just going to be calling some bash commands to tell CMake to build. We'll be calling this "DiscordBot" but you can name is whatever you want (I would advise against non-ascii characters).

\image html jenkinsproject.png

From here, just hit `Ok` and you've created your Jenkins project, Well done! From here you can add a description, change the security policy (if your jenkins is public) and whatnot.

### Automating the building process.

Scrolling down, you'll find `Build Steps` (You can also click `Build Steps` on the left). Here, you'll want to hit `Add build step` and hit `Execute shell`.

\image html buildstepjenkins.png

Inside of this, you'll want to enter this script below.

~~~~~~~~~~
# Check if the "build" directory doesn't exist (if you've not setup CMake or deleted its content).
if [ ! -d "build/" ] 
then # As it doesn't, create the build directory.
	mkdir build
	cd build
	cmake .. # Begin the CMake initialisation.
	cd ..
fi

# Commence build.
cmake --build build/
~~~~~~~~~~

This script will build your project for you and also setup CMake if you've deleted the build directory. You can change this to a build parameter if you want, meaning you can hit `Build with Parameters` and state what you'd like to do.

\image html shelljenkins.png

Now you can hit save!

### Seeing the builds work

Making sure you have your project files in the workspace directory (you can see this by pressing `Workspace` on the left, the files will automatically be pulled from GitHub if you're using the GitHub plugin), you should be able to hit `Build Now` and see a new build in the History appear. If everything went well, you should have a green tick!

\note Building can take a whilst if you haven't setup your build directory before (doing `cmake ..`), especially on less-powerful machines, so don't be alarmed!

\image html buildhistoryjenkins.png

### Running the build

Running the builds is the same as any other time, but we'll still cover it! However, we won't cover running it in background and whatnot, that part is completely down to you.

First, you need to get into the jenkins user. If you're like me and don't have the Jenkins user password, you can login with your normal login (that has sudo perms) and do `sudo su - jenkins`. Once logged in, you'll be in `/var/lib/jenkins/`. From here, you'll want to do `cd workspace/DiscordBot` (make sure to replace "DiscordBot" with your bot's name. Remember, you can tab-complete this) and then `cd build`. Now, you can simply do `./DiscordBot`!

That's it! Enjoy your automated builds!

### Possible permission issues

Sometimes, doing `./DiscordBot` can end up with an error, saying you don't have permission to execute. If that's the case, simply do `chmod +x DiscordBot` and now you can re-run `./DiscordBot`.