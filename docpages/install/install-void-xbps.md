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

To build D++ with coroutine support as an XBPS package, follow the steps below. Ensure that [xbps-src is set up](https://github.com/void-linux/void-packages?tab=readme-ov-file#quick-start) beforehand:

```bash
# Inside the void-packages root folder
git checkout master && git pull
# Modifies the configure arguments to include coroutine support (-DDPP_CORO=ON)
grep -q 'configure_args=.*-DDPP_CORO=ON' srcpkgs/dpp/template || sed -i -e 's/\(configure_args="[^"]*\)"/\1 -DDPP_CORO=ON"/' srcpkgs/dpp/template
./xbps-src -K pkg dpp
```

Then as root:
```bash
# Inside the void-packages root folder
xbps-install --repository hostdir/binpkgs dpp-devel
```

This will do the following three things:
- Update the void-packages repository to the latest commit on master
- Patch the template file of D++ to enable coroutine support
- Build an XBPS package for D++ and install it

\note Cloning the void-packages repository may take some time

\include{doc} coro_warn.dox
