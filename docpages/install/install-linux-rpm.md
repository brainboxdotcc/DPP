\page install-linux-rpm Installing from a .rpm file (RedHat, CentOS and Derivatives)

To install D++ on a system from `.rpm` using `yum` (as root):

```bash
yum install wget
wget -O dpp.rpm https://dl.dpp.dev/latest/linux-x64/rpm
yum localinstall dpp.rpm
```

This will do the following three things:

- Install `wget`
- Use `wget` to download the latest release of D++ to `dpp.rpm`
- Install `dpp.rpm` to `/usr`

You will now be able to use D++ by including its library on the command line:

```bash
g++ mybot.cpp -o mybot -ldpp
```

\include{doc} install_prebuilt_footer.dox
