\page install-arch-aur Installing from AUR (Arch Linux)

To install [D++ from AUR](https://aur.archlinux.org/packages/dpp), follow the steps below (as root):

```
git clone https://aur.archlinux.org/dpp.git
cd dpp
makepkg -si
```

or use your favorite package manager:

```sh
# example with `yay` (without root)
yay -Sy dpp
```

This will do the following three things:

- Clone the D++ AUR repository to a directory called `dpp`
- Change into the directory `dpp`
- Make a pacman package from the AUR repository for D++, and install it

You will now be able to use D++ by including its library on the command line:

```
g++ mybot.cpp -o mybot -ldpp
```
