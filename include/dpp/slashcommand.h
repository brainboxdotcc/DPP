#pragma once
#include <variant>
#include <dpp/discord.h>
#include <dpp/json_fwd.hpp>

namespace dpp {

enum command_option_type : uint8_t {
	co_sub_command = 1,
	co_sub_command_group = 2,
	co_string = 3,
	co_integer = 4,
	co_boolean = 5,
	co_user = 6,
	co_channel = 7,
	co_role = 8
};

struct command_option_choice {
	std::string name;
	std::variant<std::string, int32_t> value;

	command_option_choice() = default;
	command_option_choice(const std::string &n, std::variant<std::string, int32_t> v);
};

struct command_option {
	command_option_type type;
	std::string name;
	std::string description;
	bool required;
	std::vector<command_option_choice> choices;
	std::vector<command_option> options;

	command_option() = default;
	command_option(command_option_type t, const std::string &name, const std::string &description, bool required = false);
	command_option& add_choice(const command_option_choice &o);
	command_option& add_option(const command_option &o);
};

/**
 * @brief Represents an application command
 */
class slashcommand : public managed {
public:
	snowflake application_id;

	std::string name;

	std::string description;

	std::vector<command_option> options;

	/**
	 * @brief Construct a new slashcommand object
	 */
	slashcommand();

	/**
	 * @brief Destroy the slashcommand object
	 */
	~slashcommand();

	slashcommand& add_option(const command_option &o);

	slashcommand& set_name(const std::string &n);

	slashcommand& set_description(const std::string &d);

	slashcommand& set_application_id(snowflake i);

	/**
	 * @brief Fill object properties from JSON
	 * 
	 * @param j JSON to fill from
	 * @return slashcommand& Reference to self
	 */
	slashcommand& fill_from_json(nlohmann::json* j);

	/**
	 * @brief Build a json string for this object
	 * 
	 * @param with_id True if to include the ID in the JSON
	 * @return std::string JSON string
	 */
	std::string build_json(bool with_id = false) const;
};

/**
 * @brief A group of application slash commands
 */
typedef std::unordered_map<std::string, slashcommand> slashcommand_map;

};
