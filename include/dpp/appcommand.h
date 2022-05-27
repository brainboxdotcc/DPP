/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * Copyright 2021 Craig Edwards and D++ contributors
 * (https://github.com/brainboxdotcc/DPP/graphs/contributors)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ************************************************************************************/
#pragma once
#include <dpp/export.h>
#include <dpp/snowflake.h>
#include <dpp/managed.h>
#include <dpp/message.h>
#include <dpp/channel.h>
#include <dpp/role.h>
#include <dpp/user.h>
#include <variant>
#include <dpp/nlohmann/json_fwd.hpp>
#include <dpp/json_interface.h>

namespace dpp {

/**
 * @brief Discord limits the maximum number of replies to an autocomplete interaction to 25.
 * This value represents that maximum. interaction_response::add_autocomplete_choice does not allow
 * adding more than this number of elements to the vector.
 */
#ifndef AUTOCOMPLETE_MAX_CHOICES
	#define AUTOCOMPLETE_MAX_CHOICES 25
#endif

/**
 * @brief Represents command option types.
 * These are the possible parameter value types.
 */
enum command_option_type : uint8_t {
	/** A sub-command */
	co_sub_command = 1,
	/** A sub-command group */
	co_sub_command_group = 2,
	/** A string value */
	co_string = 3,
	/** An integer value */
	co_integer = 4,
	/** A boolean value */
	co_boolean = 5,
	/** A user snowflake id */
	co_user = 6,
	/** A channel snowflake id. Includes all channel types and categories */
	co_channel = 7,
	/** A role snowflake id */
	co_role = 8,
	/** A mentionable. Includes users and roles */
	co_mentionable = 9,
	/** Any double between -2^53 and 2^53 */
	co_number = 10,
	/** File attachment type */
	co_attachment = 11,
};

/**
 * @brief This type is a variant that can hold any of the potential
 * native data types represented by the enum above.
 * It is used in interactions.
 * 
 * std::monostate indicates an invalid parameter value, e.g. an unfilled optional parameter.
 */
typedef std::variant<std::monostate, std::string, int64_t, bool, snowflake, double> command_value;

/**
 * @brief This struct represents choices in a multiple choice option
 * for a command parameter.
 * It has both a string name, and a value parameter which is a variant,
 * meaning it can hold different potential types (see dpp::command_value)
 * that you can retrieve with std::get().
 */
struct DPP_EXPORT command_option_choice : public json_interface<command_option_choice>  {
	std::string name;	//!< Option name (1-32 chars)
	command_value value;	//!< Option value
	std::map<std::string, std::string> name_localizations; //!< Localisations of command option name

	/**
	 * @brief Construct a new command option choice object
	 */
	command_option_choice() = default;

	virtual ~command_option_choice() = default;

	/**
	 * @brief Add a localisation for this command option choice
	 * @see https://discord.com/developers/docs/reference#locales
	 * @param language Name of language, see the list of locales linked to above.
	 * @param _name name of command option choice in the specified language
	 * @return command_option_choice& reference to self for fluent chaining
	 */
	command_option_choice& add_localization(const std::string& language, const std::string& _name);

	/**
	 * @brief Construct a new command option choice object
	 *
	 * @param n name to initialise with
	 * @param v value to initialise with
	 */
	command_option_choice(const std::string &n, command_value v);

	/**
	 * @brief Fill object properties from JSON
	 *
	 * @param j JSON to fill from
	 * @return command_option_choice& Reference to self
	 */
	command_option_choice& fill_from_json(nlohmann::json* j);
};

/**
 * @brief helper function to serialize a command_option_choice to json
 *
 * @see https://github.com/nlohmann/json#arbitrary-types-conversions
 *
 * @param j output json object
 * @param choice command_option_choice to be serialized
 */
void to_json(nlohmann::json& j, const command_option_choice& choice);

/**
 * @brief A minimum or maximum value for co_number and co_integer dpp::command_option types
 */
typedef std::variant<std::monostate, int64_t, double> command_option_range;

/**
 * @brief Each command option is a command line parameter.
 * It can have a type (see dpp::command_option_type), a name,
 * a description, can be required or optional, and can have
 * zero or more choices (for multiple choice), plus options.
 * Adding options acts like sub-commands and can contain more
 * options.
 */
struct DPP_EXPORT command_option : public json_interface<command_option>  {
	command_option_type type;                    //!< Option type (what type of value is accepted)
	std::string name;                            //!< Option name (1-32 chars)
	std::string description;                     //!< Option description (1-100 chars)
	bool required;                               //!< True if this is a mandatory parameter
	bool focused;                                //!< True if the user is typing in this field, when sent via autocomplete
	command_value value;                         //!< Set only by autocomplete went sent as part of an interaction
	std::vector<command_option_choice> choices;  //!< List of choices for multiple choice command
	bool autocomplete;                           //!< True if this option supports auto completion
	std::vector<command_option> options;         //!< Sub-commands
	std::vector<channel_type> channel_types;     //!< Allowed channel types for channel snowflake id options
	command_option_range min_value;              //!< Minimum value allowed, for co_number and co_integer types only
	command_option_range max_value;              //!< Maximum value allowed, for co_number and co_integer types only
	std::map<std::string, std::string> name_localizations; //!< Localisations of command name
	std::map<std::string, std::string> description_localizations; //!< Localisations of command description


	/**
	 * @brief Construct a new command option object
	 */
	command_option() = default;

	/**
	 * @brief Destroy the command option object
	 */
	virtual ~command_option() = default;

	/**
	 * @brief Add a localisation for this slash command option
	 * @see https://discord.com/developers/docs/reference#locales
	 * @param language Name of language, see the list of locales linked to above.
	 * @param _name name of slash command option in the specified language
	 * @param _description description of slash command option in the specified language
	 * @return command_option& reference to self for fluent chaining
	 */
	command_option& add_localization(const std::string& language, const std::string& _name, const std::string& _description);

	/**
	 * @brief Construct a new command option object
	 *
	 * @param t Option type
	 * @param name Option name
	 * @param description Option description
	 * @param required True if this is a mandatory parameter
	 */
	command_option(command_option_type t, const std::string &name, const std::string &description, bool required = false);

	/**
	 * @brief Add a multiple choice option
	 *
	 * @param o choice to add
	 * @return command_option& returns a reference to self for chaining of calls
	 * @throw dpp::exception command_option is an autocomplete, so choices cannot be added
	 */
	command_option& add_choice(const command_option_choice &o);

	/**
	 * @brief Set the minimum numeric value of the option. 
	 * Only valid if the type is co_number or co_integer.
	 * @param min_v Minimum value
	 * @return command_option& return a reference to sef for chaining of calls
	 */
	command_option& set_min_value(command_option_range min_v);

	/**
	 * @brief Set the maximum numeric value of the option. 
	 * Only valid if the type is co_number or co_integer.
	 * @param max_v Maximum value
	 * @return command_option& return a reference to sef for chaining of calls
	 */
	command_option& set_max_value(command_option_range max_v);

	/**
	 * @brief Add a sub-command option
	 *
	 * @param o Sub-command option to add
	 * @return command_option& return a reference to self for chaining of calls
	 */
	command_option& add_option(const command_option &o);

	/**
	 * @brief Add channel type for option (only for co_channel type options)
	 *
	 * @param ch type to set
	 * @return command_option& return a reference to self for chaining of calls
	 */
	command_option& add_channel_type(const channel_type ch);

	/**
	 * @brief Set the auto complete state
	 * 
	 * @param autocomp True to enable auto completion for this option
	 * @return command_option& return a reference to self for chaining of calls
	 * @throw dpp::exception You attempted to enable auto complete on a command_option that has choices added to it
	 */
	command_option& set_auto_complete(bool autocomp);

	/**
	 * @brief Fill object properties from JSON. Fills options recursively.
	 *
	 * @param j JSON to fill from
	 * @return command_option& Reference to self
	 */
	 command_option& fill_from_json(nlohmann::json* j);
};

/**
 * @brief helper function to serialize a command_option to json
 *
 * @see https://github.com/nlohmann/json#arbitrary-types-conversions
 *
 * @param j output json object
 * @param opt command_option to be serialized
 */
void to_json(nlohmann::json& j, const command_option& opt);

/**
 * @brief Response types when responding to an interaction within on_interaction_create.
 * Do not use ir_acknowledge or ir::channel_message, as these are deprecated in the
 * Discord API spec. They are listed in this enum for completeness.
 */
enum interaction_response_type {
	ir_pong = 1,					//!< ACK a Ping
	ir_channel_message_with_source = 4,		//!< respond to an interaction with a message
	ir_deferred_channel_message_with_source = 5,	//!< ACK an interaction and edit a response later, the user sees a loading state
	ir_deferred_update_message = 6,			//!< for components, ACK an interaction and edit the original message later; the user does not see a loading state
	ir_update_message = 7,				//!< for components, edit the message the component was attached to
	ir_autocomplete_reply = 8,			//!< Reply to autocomplete interaction. Be sure to do this within 500ms of the interaction!
	ir_modal_dialog = 9,				//!< A modal dialog box
};

/**
 * @brief A response to an interaction, used to reply to a command and initiate
 * a message, which can be hidden from others (ephemeral) or visible to all.
 *
 * The dpp::interaction_response object wraps a dpp::message object. To set the
 * message as 'ephemeral' (e.g. only the command issuer can see it) you should
 * add the dpp::m_ephemeral flag to the dpp::message::flags field. e.g.:
 *
 * `mymessage.flags |= dpp::m_ephemeral;`
 */
struct DPP_EXPORT interaction_response : public json_interface<interaction_response>   {

	/**
	 * @brief Response type from dpp::interaction_response_type.
	 * Should be one of ir_pong, ir_channel_message_with_source,
	 * or ir_deferred_channel_message_with_source.
	 */
	interaction_response_type type;

	/**
	 * @brief A message object. This pointer is always valid
	 * while the containing interaction_response exists.
	 */
	struct message* msg;

	/**
	 * @brief Array of up to 25 autocomplete choices
	 */
	std::vector<command_option_choice> autocomplete_choices;

	/**
	 * @brief Construct a new interaction response object
	 */
	interaction_response();

	/**
	 * @brief Construct a new interaction response object
	 *
	 * @param t Type of reply
	 * @param m Message to reply with
	 */
	interaction_response(interaction_response_type t, const struct message& m);

	/**
	 * @brief Construct a new interaction response object
	 *
	 * @param t Type of reply
	 */
	interaction_response(interaction_response_type t);

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
	virtual std::string build_json(bool with_id = false) const;

	/**
	 * @brief Add a command option choice
	 * 
	 * @param achoice command option choice to add
	 * @return interaction_response& Reference to self
	 */
	interaction_response& add_autocomplete_choice(const command_option_choice& achoice);

	/**
	 * @brief Destroy the interaction response object
	 */
	virtual ~interaction_response();

};

/**
 * @brief Represents a modal dialog box response to an interaction.
 * 
 * A dialog box is a modal popup which appears to the user instead of a message. One or more
 * components are displayed on a form (the same component structure as within a dpp::message).
 * When the user submits the form an on_form_submit event is dispatched to any listeners.
 */
struct DPP_EXPORT interaction_modal_response : public interaction_response, public json_interface<interaction_modal_response> {
private:
	size_t current_row;
public:
	/**
	 * @brief Custom ID for the modal form
	 */
	std::string custom_id;

	/**
	 * @brief Title of the modal form box
	 */
	std::string title;

	/**
	 * @brief List of components. All components must be placed within
	 * an action row, each outer vector is the action row.
	 */
	std::vector<std::vector<component>> components;

	/**
	 * @brief Construct a new interaction modal response object
	 */
	interaction_modal_response();

	/**
	 * @brief Construct a new interaction modal response object
	 * 
	 * @param _custom_id Custom ID of the modal form
	 * @param _title Title of the modal form. It will be truncated to the maximum length of 45 UTF-8 characters.
	 * @param _components Components to add to the modal form
	 */
	interaction_modal_response(const std::string& _custom_id, const std::string& _title, const std::vector<component> _components = {});

	/**
	 * @brief Set the custom id
	 * 
	 * @param _custom_id custom id to set
	 * @return interaction_modal_response& Reference to self
	 */
	interaction_modal_response& set_custom_id(const std::string& _custom_id);

	/**
	 * @brief Set the title 
	 * 
	 * @param _title title to set
	 * @return interaction_modal_response& Reference to self
	 */
	interaction_modal_response& set_title(const std::string& _title);

	/**
	 * @brief Add a component to an interaction modal response
	 * 
	 * @param c component to add
	 * @return interaction_modal_response& Reference to self
	 */
	interaction_modal_response& add_component(const component& c);

	/**
	 * @brief Add a new row to the interaction modal response.
	 * @note A modal response can have a maximum of five rows.
	 * @throw dpp::logic_exception if more than five rows are attempted to be added
	 * @return interaction_modal_response& Reference to self
	 */
	interaction_modal_response& add_row();

	/**
	 * @brief Fill object properties from JSON
	 *
	 * @param j JSON to fill from
	 * @return interaction_response& Reference to self
	 */
	 interaction_modal_response& fill_from_json(nlohmann::json* j);

	/**
	 * @brief Build a json string for this object
	 * @param with_id include id in json output
	 *
	 * @return std::string JSON string
	 */
	std::string build_json(bool with_id = false) const;

	/**
	 * @brief Destroy the interaction modal response object
	 */
	virtual ~interaction_modal_response() = default;
};

/**
 * @brief Resolved snowflake ids to users, guild members, roles and channels.
 */
struct DPP_EXPORT command_resolved {
	/**
	 * @brief Resolved users
	 */
	std::map<dpp::snowflake, dpp::user> users;
	/**
	 * @brief Resolved guild members
	 */
	std::map<dpp::snowflake, dpp::guild_member> members;
	/**
	 * @brief Resolved total guild member permissions in the channel, including overwrites
	 */
	std::map<dpp::snowflake, permission> member_permissions;
	/**
	 * @brief Resolved roles
	 */
	std::map<dpp::snowflake, dpp::role> roles;
	/**
	 * @brief Resolved channels
	 */
	std::map<dpp::snowflake, dpp::channel> channels;
	/**
	 * @brief Resolved messages
	 */
	std::map<dpp::snowflake, dpp::message> messages;
	/**
	 * @brief Resolved attachments
	 */
	std::map<dpp::snowflake, dpp::attachment> attachments;
};

/**
 * @brief Values in the command interaction.
 * These are the values specified by the user when actually issuing
 * the command on a channel or in DM.
 */
struct DPP_EXPORT command_data_option {
	std::string name;                          //!< the name of the parameter
	command_option_type type;                  //!< value of ApplicationCommandOptionType
	command_value value;                       //!< Optional: the value of the pair
	std::vector<command_data_option> options;  //!< Optional: present if this option is a group or subcommand
	bool focused;                              //!< Optional: true if this option is the currently focused option for autocomplete
};

/**
 * @brief helper function to deserialize a command_data_option from json
 *
 * @see https://github.com/nlohmann/json#arbitrary-types-conversions
 *
 * @param j output json object
 * @param cdo command_data_option to be deserialized
 */
void from_json(const nlohmann::json& j, command_data_option& cdo);

/** Types of interaction in the dpp::interaction class
 */
enum interaction_type {
	it_ping = 1,			//!< ping
	it_application_command = 2,	//!< application command (slash command)
	it_component_button = 3,	//!< button click (component interaction)
	it_autocomplete = 4,		//!< Autocomplete interaction
	it_modal_submit = 5,		//!< Modal form submission
};

/**
 * @brief Right-click context menu types
 */
enum slashcommand_contextmenu_type {
    ctxm_none = 0,        //!< Undefined context menu type
    ctxm_chat_input = 1,    //!< DEFAULT, these are the slash commands you're used to
    ctxm_user = 2,        //!< Add command to user context menu
    ctxm_message = 3    //!< Add command to message context menu
};

/**
 * @brief Details of a command within an interaction.
 * This subobject represents the application command associated
 * with the interaction.
 */
struct DPP_EXPORT command_interaction {
	snowflake id;                              //!< the ID of the invoked command
	std::string name;                          //!< the name of the invoked command
	std::vector<command_data_option> options;  //!< Optional: the params + values from the user
	slashcommand_contextmenu_type type;        //!< type of the command interaction
	dpp::snowflake target_id;                  //!< Non-zero target ID for context menu actions. e.g. user id or message id whom clicked or tapped with the context menu https://discord.com/developers/docs/interactions/application-commands#user-commands
};

/**
 * @brief helper function to deserialize a command_interaction from json
 *
 * @see https://github.com/nlohmann/json#arbitrary-types-conversions
 *
 * @param j output json object
 * @param ci command_interaction to be deserialized
 */
void from_json(const nlohmann::json& j, command_interaction& ci);

/**
 * @brief Component type, either button or select
 */
enum component_type_t {
	/**
	 * @brief Button
	 */
	cotype_button = 2,
	/**
	 * @brief Option select list (drop-down)
	 */
	cotype_select = 3
};

/**
 * @brief A button click for a button component
 */
struct DPP_EXPORT component_interaction {
	/**
	 * @brief Component type
	 */
	uint8_t component_type;
	/**
	 * @brief Custom ID set when created
	 */
	std::string custom_id;
	/**
	 * @brief Possible values for a drop down list
	 */
	std::vector<std::string> values;
};

/**
 * @brief An auto complete interaction
 */
struct DPP_EXPORT autocomplete_interaction {
};

/**
 * @brief helper function to deserialize a component_interaction from json
 *
 * @see https://github.com/nlohmann/json#arbitrary-types-conversions
 *
 * @param j output json object
 * @param bi button_interaction to be deserialized
 */
void from_json(const nlohmann::json& j, component_interaction& bi);

/**
 * @brief helper function to deserialize an autocomplete_interaction from json
 *
 * @see https://github.com/nlohmann/json#arbitrary-types-conversions
 *
 * @param j output json object
 * @param ai autocomplete_interaction to be deserialized
 */
void from_json(const nlohmann::json& j, autocomplete_interaction& ai);

/**
 * @brief An interaction represents a user running a command and arrives
 * via the dpp::cluster::on_interaction_create event. This is further split
 * into the events on_form_submit, on_slashcommand, on_user_context_menu,
 * on_button_click, on_select_menu, etc.
 */
class DPP_EXPORT interaction : public managed, public json_interface<interaction>  {
public:
	snowflake application_id;                                   //!< id of the application this interaction is for
	uint8_t	type;                                               //!< the type of interaction
	std::variant<command_interaction, component_interaction, autocomplete_interaction> data; //!< Optional: the command data payload
	snowflake guild_id;                                         //!< Optional: the guild it was sent from
	snowflake channel_id;                                       //!< Optional: the channel it was sent from
	snowflake message_id;					    //!< Originating message id for context menu actions
	message msg;						    //!< Originating message for context menu actions
	guild_member member;                                        //!< Optional: guild member data for the invoking user, including permissions
	user usr;                                                   //!< Optional: user object for the invoking user, if invoked in a DM
	std::string token;                                          //!< a continuation token for responding to the interaction
	uint8_t version;                                            //!< read-only property, always 1
	command_resolved resolved;				    //!< Resolved user/role etc
	std::string locale;                                         //!< User's locale (language)
	std::string guild_locale;                                   //!< Guild's locale (language) - for guild interactions only
	cache_policy_t cache_policy;                                //!< Cache policy from cluster

	/**
	 * @brief Construct a new interaction object
	 */
	interaction();

	virtual ~interaction() = default;

	/**
	 * @brief Get the command interaction object
	 * 
	 * @throw dpp::logic_exception if the interaction is not for a command
	 * 
	 * @return command_interaction object
	 */
	command_interaction get_command_interaction() const;

	/**
	 * @brief Get the component interaction object
	 * 
	 * @throw dpp::logic_exception if the interaction is not for a component
	 * 
	 * @return component_interaction object
	 */
	component_interaction get_component_interaction() const;

	/**
	 * @brief Get the autocomplete interaction object
	 * 
	 * @throw dpp::logic_exception if the interaction is not for an autocomplete
	 * 
	 * @return autocomplete_interaction object
	 */
	autocomplete_interaction get_autocomplete_interaction() const;

	/**
	 * @brief Get the command name for a command interaction
	 * 
	 * @return std::string command interaction, or empty string if the interaction
	 * is not for a command.
	 */
	std::string get_command_name() const;

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
 * @brief helper function to deserialize an interaction from json
 *
 * @see https://github.com/nlohmann/json#arbitrary-types-conversions
 *
 * @param j output json object
 * @param i interaction to be deserialized
 */
void from_json(const nlohmann::json& j, interaction& i);

/**
 * @brief type of permission in the dpp::command_permission class
 */
enum command_permission_type {
	/**
	 * @brief Role permission
	 * 
	 */
	cpt_role = 1,
	/**
	 * @brief User permission
	 * 
	 */
	cpt_user = 2,
};

/**
 * @brief Application command permissions allow you to enable or
 * disable commands for specific users or roles within a guild
 */
class DPP_EXPORT command_permission : public json_interface<command_permission>   {
public:
	snowflake id;                  //!< the ID of the role or user
	command_permission_type type;  //!< the type of permission
	bool permission;               //!< true to allow, false, to disallow

	/**
	 * @brief Construct a new command permission object
	 */
	command_permission() = default;

	virtual ~command_permission() = default;

	/**
	 * @brief Construct a new command permission object
	 *
	 * @param id The ID of the role or user
	 * @param t The permission type
	 * @param permission True to allow, false, to disallow
	 */
	command_permission(snowflake id, const command_permission_type t, bool permission);

	/**
	 * @brief Fill object properties from JSON
	 *
	 * @param j JSON to fill from
	 * @return command_permission& Reference to self
	 */
	command_permission &fill_from_json(nlohmann::json *j);
};

/**
 * @brief helper function to serialize a command_permission to json
 *
 * @see https://github.com/nlohmann/json#arbitrary-types-conversions
 *
 * @param j output json object
 * @param cp command_permission to be serialized
 */
void to_json(nlohmann::json& j, const command_permission& cp);

/**
 * @brief Returned when fetching the permissions for a command in a guild.
 */
class DPP_EXPORT guild_command_permissions : public json_interface<guild_command_permissions>  {
public:
	snowflake id;                                 //!< the id of the command
	snowflake application_id;                     //!< the id of the application the command belongs to
	snowflake guild_id;                           //!< the id of the guild
	std::vector<command_permission> permissions;  //!< the permissions for the command in the guild

	/**
	 * @brief Construct a new guild command permissions object
	 */
	guild_command_permissions();

	virtual ~guild_command_permissions() = default;

	/**
	 * @brief Fill object properties from JSON
	 *
	 * @param j JSON to fill from
	 * @return guild_command_permissions& Reference to self
	 */
	guild_command_permissions &fill_from_json(nlohmann::json *j);

};

/**
 * @brief helper function to serialize a guild_command_permissions to json
 *
 * @see https://github.com/nlohmann/json#arbitrary-types-conversions
 *
 * @param j output json object
 * @param gcp guild_command_permissions to be serialized
 */
void to_json(nlohmann::json& j, const guild_command_permissions& gcp);

/**
 * @brief Represents an application command, created by your bot
 * either globally, or on a guild.
 */
class DPP_EXPORT slashcommand : public managed, public json_interface<slashcommand>  {
public:
	/**
	 * @brief Application id (usually matches your bots id)
	 */
	snowflake application_id;

	/**
	 * @brief Context menu type, defaults to dpp::ctxm_chat_input
	 */
	slashcommand_contextmenu_type type;

	/**
	 * @brief Command name (1-32 chars)
	 */
	std::string name;

	/**
	 * @brief Command description (1-100 chars)
	 */
	std::string description;

	/**
	 * @brief Command options (parameters)
	 */
	std::vector<command_option> options;

	/**
	 * @brief whether the command is enabled by default when the app is added to a guild.
	 * This has no effect as the default_member_permissions value is used instead.
	 * @deprecated Discord discourage use of this value and instead you should use default_member_permissions.
	 */
	bool default_permission;

	/**
	 * @brief command permissions
	 * @deprecated Discord discourage use of this value and instead you should use default_member_permissions.
	 */
	std::vector<command_permission> permissions;

	/**
	 * @brief autoincrementing version identifier updated during substantial record changes
	 */
	snowflake version;

	/**
	 * @brief Localisations of command name
	 */
	std::map<std::string, std::string> name_localizations;

	/**
	 * @brief Localisations of command description
	 */
	std::map<std::string, std::string> description_localizations;

	/**
	 * @brief The default permissions of this command on a guild.
	 * D++ defaults this to p_use_application_commands.
	 * @note You can set it to 0 to disable the command for everyone except admins by default
	 */
	permission default_member_permissions;

	/**
	 * @brief True if this command should be allowed in a DM
	 * D++ defaults this to false. Cannot be set to true in a guild
	 * command, only a global command.
	 */
	bool dm_permission;

	/**
	 * @brief Construct a new slashcommand object
	 */
	slashcommand();

	/**
	 * @brief Construct a new slashcommand object
	 * 
	 * @param _name Command name
	 * @param _description Command description
	 * @param _application_id Application id (usually the bot's user id)
	 */
	slashcommand(const std::string &_name, const std::string &_description, const dpp::snowflake _application_id);

	/**
	 * @brief Destroy the slashcommand object
	 */
	virtual ~slashcommand();

	/**
	 * @brief Add a localisation for this slash command
	 * @see https://discord.com/developers/docs/reference#locales
	 * @param language Name of language, see the list of locales linked to above.
	 * @param _name name of slash command in the specified language
	 * @param _description description of slash command in the specified language
	 * @return slashcommand& reference to self for fluent chaining
	 */
	slashcommand& add_localization(const std::string& language, const std::string& _name, const std::string& _description);

	/**
	 * @brief Set the dm permission for the command
	 * 
	 * @param dm true to allow this command in dms
	 * @return slashcommand& reference to self
	 */
	slashcommand& set_dm_permission(bool dm);

	/**
	 * @brief Set the default permissions of the slash command
	 * 
	 * @param defaults default permissions to set. This is a permission bitmask
	 * @note You can set it to 0 to disable the command for everyone except admins by default
	 *
	 * @return slashcommand& reference to self
	 */
	slashcommand& set_default_permissions(uint64_t defaults);

	/**
	 * @brief Add an option (parameter)
	 *
	 * @param o option (parameter) to add
	 * @return slashcommand& reference to self for chaining of calls
	 */
	slashcommand& add_option(const command_option &o);

	/**
	 * @brief Set the type of the slash command (only for context menu entries)
	 * 
	 * @param _type Type of context menu entry this command represents
	 * @note If the type is dpp::ctxm_chat_input, the command name will be set to lowercase.
	 * @return slashcommand& reference to self for chaining of calls
	 */
	slashcommand& set_type(slashcommand_contextmenu_type _type);

	/**
	 * @brief Set the name of the command
	 *
	 * @param n name of command
	 * @note The maximum length of a command name is 32 UTF-8 codepoints.
	 * If your command name is longer than this, it will be truncated.
	 * The command name will be set to lowercase when the type is the default dpp::ctxm_chat_input.
	 * @return slashcommand& reference to self for chaining of calls
	 */
	slashcommand& set_name(const std::string &n);

	/**
	 * @brief Set the description of the command
	 *
	 * @param d description
	 * @note The maximum length of a command description is 100 UTF-8 codepoints.
	 * If your command description is longer than this, it will be truncated.
	 * @return slashcommand& reference to self for chaining of calls
	 */
	slashcommand& set_description(const std::string &d);

	/**
	 * @brief Set the application id of the command
	 *
	 * @param i application id
	 * @return slashcommand& reference to self for chaining of calls
	 */
	slashcommand& set_application_id(snowflake i);

	/**
	 * @brief Adds a permission to the command
	 *
	 * @param p permission to add
	 * @return slashcommand& reference to self for chaining of calls
	 * @deprecated Discord discourage use of this value and instead you should use default_member_permissions.
	 */
	slashcommand& add_permission(const command_permission& p);

	/**
	 * @brief Disable default permissions, command will be unusable unless
	 *        permissions are overridden with add_permission and
	 *        dpp::guild_command_edit_permissions
	 *
	 * @return slashcommand& reference to self for chaining of calls
	 * @deprecated Discord discourage use of this value and instead you should use default_member_permissions.
	 */
	slashcommand& disable_default_permissions();

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
 * @brief helper function to serialize a slashcommand to json
 *
 * @see https://github.com/nlohmann/json#arbitrary-types-conversions
 *
 * @param j output json object
 * @param cmd slashcommand to be serialized
 */
void to_json(nlohmann::json& j, const slashcommand& cmd);

/**
 * @brief A group of application slash commands
 */
typedef std::unordered_map<snowflake, slashcommand> slashcommand_map;

/**
 * @brief A group of guild command permissions
 */
typedef std::unordered_map<snowflake, guild_command_permissions> guild_command_permissions_map;

};
