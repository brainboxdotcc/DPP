\page buildmeson Build a Discord Bot Using Meson

## 1. Toolchain

Before compiling, you will need to install `meson` on your system. To be sure that `meson` is installed, you can type the following command:

```bash
meson --version
0.63.2
```

## 2. Create a Meson project

First, you'll need to go ahead and create an empty directory, we'll call it `meson-project`.

Then, run this command:

```bash
meson init -l cpp
```

## 3. Configuring Your Meson Project

Add the following line after the `project()` line in your `meson.build` file.

```yml
dpp = dependency('dpp')
```

Add the following line in the executable section of your `meson.build` file.

```yml
dependencies: [dpp]
```

Change the `cpp_std` value in the `project()` to `c++17`. Your `meson.build` should look like this:

your meson.build should look like this.
~~~~~~~~~~~~~~yml
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

To build a Meson project, run the following:

```bash
meson setup builddir
meson compile -C builddir
```

Now, your Meson project should be all setup!

**Have fun!**
