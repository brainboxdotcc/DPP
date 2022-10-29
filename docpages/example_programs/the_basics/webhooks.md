\page webhooks Webhooks

Webhooks are a simple way to post messages from other apps and websites into Discord. They allow getting automated messages and data updates sent to a text channel in your server. [Read more](https://support.discord.com/hc/en-us/articles/228383668) in this article about Webhooks.

The following code shows how to send messages in a channel using a webhook.

~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main()
{
    dpp::cluster bot(""); // normally, you put your bot token in here. But to just run a webhook its not required

    bot.on_log(dpp::utility::cout_logger());

    /* construct a webhook object using the URL you got from Discord */
    dpp::webhook wh("https://discord.com/api/webhooks/833047646548133537/ntCHEYYIoHSLy_GOxPx6pmM0sUoLbP101ct-WI6F-S4beAV2vaIcl_Id5loAMyQwxqhE");

    /* send a message with this webhook */
    bot.execute_webhook_sync(wh, dpp::message("Have a great time here :smile:"));

    return 0;
}
~~~~~~~~~~

The above is just a very simple example. You can also send embed messages. All you have to do is to add an embed to the message you want to send. If you want to, you can also send it into a thread.
