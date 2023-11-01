\page oggopus Streaming Ogg Opus file

This example shows how to stream an Ogg Opus file to a voice channel. This example requires some additional dependencies, namely `libogg` and `opusfile`.

\include{cpp} oggopus.cpp

You can compile this example using the following command

```bash
c++ /path/to/source.cc -ldpp -lopus -lopusfile -logg -I/usr/include/opus
```

## Using liboggz

You can use `liboggz` to stream an Ogg Opus file to discord voice channel.
`liboggz` provides higher level abstraction and useful APIs. Some features `liboggz` provides include: seeking and timestamp interpretation.
Read more on the [documentation](https://www.xiph.org/oggz/doc/).

\include{cpp} oggopus2.cpp

You can compile this example using the following command:

```bash
c++ /path/to/source.cc -ldpp -loggz
```