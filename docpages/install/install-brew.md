\page install-brew Installing from Homebrew (OSX)

To install D++ from brew, follow the steps below:

```bash
brew install libdpp

brew link libdpp
```

This command will install libdpp and setup links so it can be used correctly.

You will now be able to use D++ by including its library on the command line:

```bash
clang++ -std=c++17 -I/opt/homebrew/include -ldpp mybot.cpp -o mybot
```

\note A crucial part of this command is `-I/opt/homebrew/include`. AppleClang should automatically have `/opt/homebrew/lib` added as a way to detect library (if it doesn't, you can do `-L/opt/homebrew/lib` with this command), however, AppleClang does not auto-detect includes from homebrew. This means, D++ needs you to link the includes folder.

\include{doc} install_prebuilt_footer.dox

## Uninstalling & Unlinking

To unlink and uninstall dpp, run
```bash
brew unlink libdpp
brew uninstall libdpp
```

Then, double check inside `/opt/homebrew/lib` and `/opt/homebrew/include` to make sure that libdpp does not exist. If it does, remove files/folders relating to libdpp.

**Have fun!**