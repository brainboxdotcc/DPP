\page install-void-xbps Installing from XBPS (Void Linux)

To install [D++ from XBPS source packages collection](https://github.com/void-linux/void-packages/blob/master/srcpkgs/dpp/template), follow the step below (as root):

```bash
xbps-install -Sy dpp-devel
```

This will install D++ development files and related libraries


You will now be able to use D++ by including its library on the command line:

```bash
g++ mybot.cpp -o mybot -ldpp
```

\include{doc} install_prebuilt_footer.dox
