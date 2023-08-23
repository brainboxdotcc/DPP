\page install-linux-deb Installing from a .deb file (Debian, Ubuntu, Derivatives)

To install D++ on a system from `.deb` using `dpkg` (as root):

```
apt install wget
wget -O dpp.deb https://dl.dpp.dev/
dpkg -i dpp.deb
```

This will do the following three things:

- Install `wget`
- Use `wget` to download the latest release of D++ to `dpp.deb`
- Install `dpp.deb` to `/usr`

You will now be able to use D++ by including its library on the command line:

```
g++ mybot.cpp -o mybot -ldpp
```