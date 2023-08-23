\page buildmeson Build a Discord Bot using Meson

## 1. Toolchain

Before compiling, you will need to install `meson` on your system.
To be sure that `meson` is installed, you can type the following command:

    $ meson --version
    0.63.2

## 2. Create a Meson project

In an empty directory.

    - your project/

run the command 

    $ meson init -l cpp

## 3. Configuring your Meson project

add the following line after the `project()` line in your `meson.build` file.

    dpp = dependency('dpp')

add the following line in the executable section of your `meson.build` file.

    dependencies: [dpp]

change the `cpp_std` value in the `project()` to `c++17`

your meson.build should look like this.
~~~~~~~~~~~~~~
project('discord-bot', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3',
                     'cpp_std=c++17'])

dpp = dependency('dpp')


exe = executable('discord', 'discord_bot.cpp',
  install : true, dependencies: [dpp])

test('basic', exe)

~~~~~~~~~~~~~~

Meson automatically generates a cpp for your project. And a test suite.

## 4. Building

To build a meson project run

    $ meson setup builddir
    $ meson compile -C builddir