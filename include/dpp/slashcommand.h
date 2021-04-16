#pragma once
#include <variant>
#include <dpp/discord.h>
#include <dpp/json_fwd.hpp>
#include <dpp/message.h>

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

typedef std::variant<std::string, int32_t, bool, snowflake> command_value;

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

enum interaction_response_type {
	ir_pong = 1,					//< ACK a Ping
	ir_acknowledge = 2,				//< DEPRECATED ACK a command without sending a message, eating the user's input
	ir_channel_message = 3,				//< DEPRECATED respond with a message, eating the user's input
	ir_channel_message_with_source = 4,		//< respond to an interaction with a message
	ir_deferred_channel_message_with_source = 5	//< ACK an interaction and edit a response later, the user sees a loading state
};

struct interaction_response {
	interaction_response_type type;
	class message* msg;

	interaction_response();

	/**
	 * @brief Fill object properties from JSON
	 * 
	 * @param j JSON to fill from
	 * @return interaction_response& Reference to self
	 */
	interaction_response& fill_from_json(nlohmann::json* j);

	/**
	 * @brief Build a json string for this object
	 * 
	 * @return std::string JSON string
	 */
	std::string build_json() const;

	~interaction_response();

};

struct command_resolved {

};

struct command_data_option {
	std::string				name;		//< the name of the parameter
	command_option_type			type;		//< value of ApplicationCommandOptionType
	command_value				value;		//< Optional: the value of the pair
	std::vector<command_data_option>	options;	//< Optional: present if this option is a group or subcommand
};

struct command_interaction {
	snowflake id;					//< the ID of the invoked command
	std::string name;				//< the name of the invoked command
	command_resolved resolved;			//< Optional: converted users + roles + channels
	std::vector<command_data_option> options;	//< Optional: the params + values from the user
};

class interaction : public managed {
public:
	snowflake	id;			//< id of the interaction
	snowflake	application_id;		//< id of the application this interaction is for
	uint8_t		type;			//< the type of interaction
	command_interaction data;		//< Optional: the command data payload
	snowflake	guild_id;		//< Optional: the guild it was sent from
	snowflake	channel_id;		//< Optional: the channel it was sent from
	guild_member	member;			//< Optional: guild member data for the invoking user, including permissions
	user		usr;			//< Optional: user object for the invoking user, if invoked in a DM
	std::string	token;			//< a continuation token for responding to the interaction
	uint8_t		version;		//< read-only property, always 1

	/**
	 * @brief Fill object properties from JSON
	 * 
	 * @param j JSON to fill from
	 * @return interaction& Reference to self
	 */
	interaction& fill_from_json(nlohmann::json* j);

	/**
	 * @brief Build a json string for this object
	 * 
	 * @param with_id True if to include the ID in the JSON
	 * @return std::string JSON string
	 */
	std::string build_json(bool with_id = false) const;

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
