\page stream-mp3-discord-bot Streaming MP3 Files

To stream MP3 files via D++ you need to link an additional dependency to your bot, namely `libmpg123`. It is relatively simple when linking this library to your bot to then decode audio to PCM and send it to the dpp::discord_voice_client::send_audio_raw function as shown below:

\include{cpp} mp3.cpp

To compile this program you must remember to specify `libmpg123` alongside `libdpp` in the build command, for example:

```bash
g++ -std=c++17 -o musictest musictest.cpp -lmpg123 -ldpp
```

\include{doc} install_prebuilt_footer.dox
