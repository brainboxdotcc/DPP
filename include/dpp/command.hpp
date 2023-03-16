#include <string>
#include <vector>
#include <unordered_map>
#include <dpp/dpp.h>

namespace dpp::ebw{
	struct Option {
		std::string id = "option id";
		std::string display = "option display";
		std::string response = "option response";
	};
	struct Argument {
		std::string id = "argument id";
		std::string display = "argument display";
		std::string response = "argument response";
		std::unordered_map<std::string, Option> option_map = {};
		std::vector<Option> options = {};
	};
	struct Evt;
	struct Command {
		std::string id = "command id";
		std::string display = "command display";
		std::string response = "command response";
		std::vector<Argument> arguments = {};
		std::unordered_map<std::string, Argument> argument_map = {};
		using execute_fp = void (*)(dpp::cluster& bot, const dpp::slashcommand_t& event, Evt cmd);
		execute_fp execute;
	};
	struct Evt {
		std::vector<std::string> args = {};
		std::unordered_map<std::string, std::string> arg = {};
		Command cmd;
	};
	void Cmd(dpp::cluster& bot, const dpp::slashcommand_t& event, Command& command) {
		Evt evt;
		evt.cmd = command;
		for (int arg_i = 0; arg_i < command.arguments.size(); arg_i++) {
			Argument& current_arg = command.arguments[arg_i];
			std::string choice = std::get<std::string>(event.get_parameter(current_arg.id));
			evt.args.push_back(choice);
			evt.arg[current_arg.id] = choice;
		}
		command.execute(bot, event, evt);
	}
	std::unordered_map<std::string, Command> command_map;
	void init_commands(dpp::cluster& bot, std::vector<Command>& commands_list) {
		for (int command_i = 0; command_i < commands_list.size(); command_i++) {
			Command& command = commands_list[command_i];
			command_map[command.id] = command;
			dpp::slashcommand newcommand(command.id, command.display, bot.me.id);
			for (int arg_i = 0; arg_i < command.arguments.size(); arg_i++) {
				Argument& argument = command.arguments[arg_i];
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


