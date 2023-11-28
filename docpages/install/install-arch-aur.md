\page install-arch-aur Installing from AUR (Arch Linux)

To install [D++ from AUR](https://aur.archlinux.org/packages/dpp), follow the steps below (as root):

```bash
git clone https://aur.archlinux.org/dpp.git
cd dpp
makepkg -si
```

or use your favourite package manager:

```bash
# example with `yay` (without root)
yay -Syu dpp
```

\note The package is currently outdated. We are looking for a new maintainer. For now, please use `dpp-git` or build it from \ref buildlinux "source".

This will do the following three things:

- Clone the D++ AUR repository to a directory called `dpp`
- Change into the directory `dpp`
- Make a pacman package from the AUR repository for D++, and install it

You will now be able to use D++ by including its library on the command line:

```bash
g++ mybot.cpp -o mybot -ldpp
```

\include{doc} install_prebuilt_footer.dox
