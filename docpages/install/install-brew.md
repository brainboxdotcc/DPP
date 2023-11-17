\page install-brew Installing from Homebrew (OSX)

To install D++ from brew, follow the steps below:

```bash
brew install libdpp

brew link libdpp
```

You will now be able to use D++ by including its library on the command line:

```bash
clang++ -std=c++17 -L/opt/homebrew/lib -I/opt/homebrew/include -ldpp mybot.cpp -o mybot
```

\include{doc} install_prebuilt_footer.dox
