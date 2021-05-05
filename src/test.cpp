#include <dpp/dpp.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <dpp/message.h>

using json = nlohmann::json;

int main()
{
    dpp::cluster bot("", dpp::i_default_intents | dpp::i_guild_members, 1);

    bot.on_ready([&bot](const dpp::ready_t & event) {
        std::cout << "Logged in as " << bot.me.username << "!\n";
    });

    bot.on_log([](const dpp::log_t & event) {
        if (event.severity > dpp::ll_trace) {
            std::cout << event.message << "\n";
	    }
    });

    bot.on_message_create([&bot](const dpp::message_create_t & event) {
        if (event.msg->content == "!ping2") {
			json button_in_actionbar_json = {
				{"type", 2},
				{"label", "Click me!"},
				{"style", 1},
				{"custom_id", "button_click_one_my_data"},
				{"disabled", false}
			};
			json button_json = {
			{"type", 1}
			};
			button_json["components"] = { button_in_actionbar_json };
			dpp::component button_component;
			button_component.fill_from_json(&button_json);
			std::vector<dpp::component> action_bars;
			action_bars.push_back(button_component);
			dpp::message m(event.msg->channel_id, "this text has buttons", action_bars);
	    std::cout << m.build_json() << "\n";

	    bot.message_create(m, [](const dpp::confirmation_callback_t & callback) {
		std::cout << callback.http_info.body << "\n";
            });
        } else if (event.msg->content == "!ping4") {
			dpp::message m(event.msg->channel_id, "cock");
			bot.message_create(m, [](const dpp::confirmation_callback_t & callback) {
				std::cout << callback.http_info.body << "\n";
            });
		}
    });

    bot.start(false);

    return 0;
}
