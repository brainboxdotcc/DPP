\page building-a-cpp-discord-bot-in-repl Creating a Discord bot in Replit

@note There is a premade repl, ready for use which was built using the steps above. If you wish to use this repl simply [visit this github repository](https://github.com/alanlichen/dpp-on-repl) and click the "Run on Replit" button. Then, follow the steps in the README file.

To build a D++ bot in a Replit instance, follow these steps. These steps are slightly more convoluted than installing D++ into a standard container as we don't have access to root in the conventional way or write access to any files outside of our home directory in a repl. This guide sidesteps the issue by locally extracting a libdpp deb file installer, and referencing the local dependencies from the command-line.

1. Use wget, or the upload button, to get the precompiled x64 release into your repl as a file, e.g. `wget -O libdpp.deb https://dl.dpp.dev/latest`
2. Extract this deb file using `dpkg`:
```
dpkg -x libdpp.deb .
```
3. Compile your bot, note that you should be sure to include the `pthread` library explicitly and reference the extracted dpp installation you just put into the repl:
```
g++ -o bot main.cpp -ldpp -lpthread -L./usr/lib -I./usr/include -std=c++17
```
4. Run your bot! Note that you will need to set `LD_PRELOAD` to reference `libdpp.so` as it will be located in `$HOME` and not `/usr/lib`:
```
LD_PRELOAD=./usr/lib/libdpp.so ./bot
```

Now that your bot is running, you have to keep it online. Replit automatically puts repls to sleep after some time, so you will need to ping a webserver. Unfortunately, Replit is sometimes limiting, and this is one of the only free workarounds to this issue.

1. Start a http server. This can be through any webserver, but as a simple solution, use python's built in http.server:
```
python3 -m http.server
```
2. Create an `index.html` file with anything inside it for the server to serve.
3. Go to [uptimerobot.com](https://uptimerobot.com/) and create an account if you don't have one.
4. After verifying your account, click "Add New Monitor".
+ For Monitor Type, select "HTTP(s)"
+ In Friendly Name, put the name of your bot
+ For your url, copy the url of the new website that repl is serving for you
+ Select any alert contacts you want, then click "Create Monitor"
Here is an example of a possible uptimerobot configuration:

\image html uptimerobot.png

## Troubleshooting

If the bot fails to start and instead you receive an error message about being banned from the Discord API, there is little to be done about this. These bans are temporary but because Replit is a shared platform, you share an IP address with many thousands of bots, some abusive and some badly written. This will happen often and is outside of the control of yourself and us. However, you can try to migitate this by typing `kill 1` in the shell. This is not guaranteed to work, and you might need to try it a few times. If it still does not work, then we recommend instead you obtain some affordable non-free hosting instead.

If your bot continues to fall asleep even though you have a server, we advise you to double check that no errors are happening, and if the server is being pinged. If that still does not work, we again recommend you to obtain some affordable non-free hosting.
