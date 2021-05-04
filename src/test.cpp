#include <dpp/dpp.h>
#include <dpp/component.h>
#include <iostream>
#include <nlohmann/json.hpp>

int main()
{
    dpp::cluster bot("ODI4NDMyMjg5Nzc0OTYwNjcx.YGpfsQ.bte6_hT9rRhR3lkhLX8N-fh70RU");
    bot.on_ready([&bot](const dpp::ready_t & event) {
        std::cout << "Logged in as " << bot.me.username << "!\n";
    });
    bot.on_message_create([&bot](const dpp::message_create_t & event) {
        if (event.msg->content == "!ping") {
			json button_json = {
			{"type", 1},
			{"components", json::array(
				{
				{"type", 2},
				{"label", "Click me!"},
				{"style", 1},
				{"custom_id", "button_click_one_my_data"},
				{"disabled", false}
				}
			)
			}
			};
			dpp::component button_component;
			button_component.fill_from_json(button_json);
			std::cout << button_component.build_json() << "!\n";
            bot.message_create(dpp::message(event.msg->channel_id, "this text has buttons", button_component));
        }
    });
    bot.start(false);
    return 0;
}