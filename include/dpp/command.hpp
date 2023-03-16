#include <string>
#include <vector>
#include <unordered_map>

namespace EBW{
	struct Option {
		std::string id;
		std::string display;
		std::string response;
	};
	struct Argument {
		std::string id;
		std::string display;
		std::unordered_map<std::string, Option> option_map;
		std::vector<Option> options;
	};
	struct Command {
		std::string id;
		std::string display;
		std::vector<Argument> args;
		std::unordered_map<std::string, Argument> argument_map;
		using execute_fp = void (*)(dpp::cluster&, const dpp::slashcommand_t&, EBW::Command& command);
		execute_fp execute;
	};
	std::unordered_map<std::string, Command> command_map;
	void init_commands(dpp::cluster& bot, std::vector<Command>& commands_list) {
		for (int command_i = 0; command_i < commands_list.size(); command_i++) {
			Command& command = commands_list[command_i];
			command_map[command.id] = command;
			dpp::slashcommand newcommand(command.id, command.display, bot.me.id);
			for (int arg_i = 0; arg_i < command.args.size(); arg_i++) {
				Argument& argument = command.args[arg_i];
				command_map[command.id].argument_map[argument.id] = argument;
				dpp::command_option newargument(dpp::co_string, argument.id, argument.display, true);
				for (int option_i = 0; option_i < argument.options.size(); option_i++) {
					Option& option = argument.options[option_i];
					command_map[command.id].argument_map[argument.id].option_map[option.id] = option;
					newargument.add_choice(dpp::command_option_choice(option.display, std::string(option.id)));
				}
				newcommand.add_option(newargument);
			}
			bot.global_command_create(newcommand);
			std::cout << "\nCreated command " << command.id << ".\n";
		}
	};
	std::vector<Command> commands;
};
