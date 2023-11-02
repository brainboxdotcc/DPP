\page install-xmake Installing from XMake

To install D++ on a project from XMake:

- Ensure XMake [is correctly installed](https://xmake.io/#/guide/installation)
- Create a new XMake project if you haven't already one, using `xmake init <project_name>`
- Update the `xmake.lua` file by adding the `dpp` package, below the minimum configuration:

~~~~~~~~~~~lua
add_rules("mode.debug", "mode.release")

add_requires("dpp")

target("test-bot")
    set_kind("binary")
    add_files("src/*.cpp")

    add_packages("dpp")
~~~~~~~~~~~

- Finally, run `xmake build` to download dependencies and build the project