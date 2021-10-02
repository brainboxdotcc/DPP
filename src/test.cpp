#undef DPP_BUILD
#include <dpp/dpp.h>
#include <dpp/fmt/format.h>
 
int main()
{
    dpp::cluster bot("token");
 
    /* The interaction create event is fired when someone issues your commands */
    bot.on_interaction_create([&bot](const dpp::interaction_create_t & event) {
        if (event.command.type == dpp::it_application_command) {
            dpp::command_interaction cmd_data = std::get<dpp::command_interaction>(event.command.data);
            /* Check which command they ran */
            if (cmd_data.name == "blep") {
                /* Fetch a parameter value from the command parameters */
                std::string animal = std::get<std::string>(event.get_parameter("animal"));
                /* Reply to the command. There is an overloaded version of this
                * call that accepts a dpp::message so you can send embeds.
                */
                event.reply(dpp::ir_channel_message_with_source, fmt::format("Blep! You chose {}", animal));
            }
        }
    });
 
    bot.on_ready([&bot](const dpp::ready_t & event) {
 
        dpp::slashcommand newcommand;
        /* Create a new global command on ready event */
        newcommand.set_name("blep")
            .set_description("Send a random adorable animal photo")
            .set_application_id(bot.me.id)
            .add_option(
                dpp::command_option(dpp::co_string, "animal", "The type of animal", true).
                    add_choice(dpp::command_option_choice("Dog", std::string("animal_dog"))).
                    add_choice(dpp::command_option_choice("Cat", std::string("animal_cat"))).
                    add_choice(dpp::command_option_choice("Penguin", std::string("animal_penguin")
                )
            )
        );
 
        /* Register the command */
        bot.global_command_create(newcommand);
    });
 
    bot.start(false);
 
    return 0;
}