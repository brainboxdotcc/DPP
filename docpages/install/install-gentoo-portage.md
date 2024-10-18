\page install-gentoo-portage Installing with Portage (Gentoo)

D++ is available on [GURU](https://wiki.gentoo.org/wiki/Project:GURU). To install D++, you must first enable the GURU repository.
To do so, execute the following commands (as root):

```bash
emerge --ask app-eselect/eselect-repository
eselect repository enable guru
emaint sync --repo guru
```

This enables the GURU repository, which consists of user-contributed packages, such as D++!

If you wish, you may enable coroutine and voice support through USE flags. To do so, using your text editor of choice, add the following line to `/etc/portage/package.use/dpp` (as root):

```
dev-cpp/dpp voice coro
```
> You may choose between voice, coro, or both, just pick and choose!

You will now be able to use D++ by including its library on the command line:

```bash
g++ mybot.cpp -o mybot -ldpp
```

\include{doc} install_prebuilt_footer.dox
