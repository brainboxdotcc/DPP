\page joinvc Join or Switch to the Voice Channel of the User Issuing a Command

When a user issues a command you may want to join their voice channel, e.g. in a music bot. If you are already on the same voice channel, the bot should do nothing (but be ready to instantly play audio) and if the user is on a different voice channel, the bot should switch to it. If the user is on no voice channel at all, this should be considered an error. This example shows how to do this.

\note Please be aware this example sends no audio, but indicates clearly in the comments where and how you should do so.

\include{cpp} join_voice.cpp