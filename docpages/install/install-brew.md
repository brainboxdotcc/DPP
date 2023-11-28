\page install-brew Installing from Homebrew (OSX)

To install D++ from brew, follow the steps below:

```bash
brew install libdpp
brew link libdpp
```

This command will install libdpp and setup links.

If it can't detect libdpp, please do `brew update` and repeat the steps above.

You will now be able to use D++ by including its library on the command line:

```bash
clang++ -std=c++17 -L/opt/homebrew/lib -I/opt/homebrew/include -ldpp mybot.cpp -o mybot
```

\include{doc} install_prebuilt_footer.dox

## Uninstalling & Unlinking

To unlink and uninstall dpp, run
```bash
brew unlink libdpp
brew uninstall libdpp
```

\note As a precaution, double check inside `/opt/homebrew/lib` and `/opt/homebrew/include` to make sure that libdpp does not exist. If it does, remove files/folders relating to libdpp. If there are files left here and you don't remove them, you may see issues arise with different versions of D++. Sometimes, it may still act like some version of D++ exists, if it does that then please restart your system!

**Have fun!**