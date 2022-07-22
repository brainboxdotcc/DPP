\page discord-application-command-file-upload Using file parameters for application commands (slash commands)

The program below demonstrates how to use the 'file' type parameter to an application command (slash command).
You must first get the file_id via std::get, and then you can find the attachment details in the 'resolved'
section, `event.command.resolved`.

The file is uploaded to Discord's CDN so if you need it locally you should fetch the `.url` value, e.g. by using
something like `dpp::cluster::request()`.

~~~~~~~~~~~~~~~~{.cpp}
#include <dpp/dpp.h>

int main()
{
        dpp::cluster bot("token");

        bot.on_log(dpp::utility::cout_logger());

        bot.on_slashcommand([&bot](const dpp::slashcommand_t & event) {
                if (event.command.type == dpp::it_application_command) {
                        dpp::command_interaction cmd_data = std::get<dpp::command_interaction>(event.command.data);
                        if (cmd_data.name == "show") {
                                dpp::snowflake file_id = std::get<dpp::snowflake>(event.get_parameter("file"));
                                auto iter = event.command.resolved.attachments.find(file_id);
                                if (iter != event.command.resolved.attachments.end()) {
                                        const dpp::attachment& att = iter->second;
                                        event.reply(att.url);
                                }
                        }
                }
        });

        bot.on_ready([&bot](const dpp::ready_t & event) {

                if (dpp::run_once<struct register_bot_commands>()) {
                        dpp::slashcommand newcommand("show", "Show an uploaded file", bot.me.id);

                        newcommand.add_option(dpp::command_option(dpp::co_attachment, "file", "Select an image"));

                        bot.global_command_create(newcommand);
                }
        });

        bot.start(dpp::st_wait);

        return 0;
}
~~~~~~~~~~~~~~~~
