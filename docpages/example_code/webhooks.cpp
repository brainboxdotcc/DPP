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
