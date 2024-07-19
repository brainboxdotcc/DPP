\page editing_message_after_click Editing The Message From a Button Click

\note This page expects you to be familiar with Button Clicks and extends from the \ref components page. Please visit the \ref components page if you are not already familiar with Button Clicks.

Editing messages where you had a button click can be done in a couple ways.

If you want to edit the message that had the buttons on, instead of doing `event.reply("message");`, you would do `event.reply(dpp::ir_update_message, "message");`, like so:

\note You are still limited to the default interaction time (3 seconds) this way. Read on if you need more time!

\include{cpp} editing_message_after_click.cpp

However, if you're going to take longer than 3 seconds to respond, you need to tell Discord to wait. The usual method is `event.thinking(true);` and then `event.edit_response("I have thought long and hard about my actions.");`, however, `event.thinking()` will create a new response if called from `on_button_click`, meaning you can no longer respond to the original response as you already did a response!

Instead, you want to do `event.reply(dpp::ir_deferred_channel_message_with_source, "");` to tell Discord that you intend on editing the message that the button click came from, but you need time. The user will be informed that you've processed the button click (as required) via a loading state and then you have 15 minutes to do everything you need. To then edit the message, you need to do `event.edit_response("new message!");`, like so:

\note If you want to silently acknowledge the button click (no thinking message), replace dpp::ir_deferred_channel_message_with_source with dpp::ir_deferred_update_message. You will still have 15 minutes to make a response.

\include{cpp} editing_message_after_click2.cpp
