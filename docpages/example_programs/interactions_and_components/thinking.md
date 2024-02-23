\page thinking Thinking

A common mistake people do is use `event.thinking` with `event.reply`, however, they always run into the `Interaction has already been acknowledged.` error! The reason for this is because `event.thinking` is a response to the interaction, meaning you have acknowledged it! You should use dpp::interaction_create_t::edit_original_response instead.

Below is an example, showing how you should properly use the thinking method.

\include{cpp} thinking.cpp

This will make the bot think briefly, then change the response to "thonk"!
