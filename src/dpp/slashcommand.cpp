#include <dpp/slashcommand.h>
#include <dpp/discordevents.h>
#include <dpp/discord.h>
#include <nlohmann/json.hpp>

namespace dpp {

using json = nlohmann::json;

slashcommand::slashcommand() : managed() 
{
}

slashcommand::~slashcommand() {
}

slashcommand& slashcommand::fill_from_json(nlohmann::json* j) {
	id = SnowflakeNotNull(j, "id");
	return *this;
}

std::string slashcommand::build_json(bool with_id) const {
	json j;
	if (with_id) {
		j["id"] = std::to_string(id);
	}
	j["name"] = name;
	j["description"] = description;
	if (options.size()) {
		j["options"] = json();
		json& o = j["options"];
		for (auto & opt : options) {
			json n;
			n["name"] = opt.name;
			n["description"] = opt.description;
			n["type"] = opt.type;
			n["required"] = opt.required;

			if (opt.choices.size()) {
				n["choices"] = json();
				json &c = n["choices"];
				for (auto & choice : opt.choices) {
					json t;
					t["name"] = choice.name;
					if (std::holds_alternative<int>(choice.value)) {
						t["value"] = std::get<int>(choice.value);
					} else {
						t["value"] = std::get<std::string>(choice.value);
					}
					c.push_back(t);
				}
			}

			if (opt.options.size()) {
				n["options"] = json();
				json &c = n["options"];
				for (auto & subcommand : opt.options) {
					json o;
					o["name"] = opt.name;
					o["description"] = opt.description;
					o["type"] = opt.type;
					o["required"] = opt.required;
					if (opt.choices.size()) {
						o["choices"] = json();
						json &v = o["choices"];
						for (auto & choice : opt.choices) {
							v["name"] = choice.name;
							if (std::holds_alternative<int>(choice.value)) {
								v["value"] = std::get<int>(choice.value);
							} else {
								v["value"] = std::get<std::string>(choice.value);
							}
						}
						v.push_back(o);
					}
					c.push_back(n);
				}
			}
			o.push_back(n);
		}
	}
	return j.dump();
}

slashcommand& slashcommand::set_name(const std::string &n) {
	name = n;
	return *this;
}

slashcommand& slashcommand::set_description(const std::string &d) {
	description = d;
	return *this;
}

slashcommand& slashcommand::set_application_id(snowflake i) {
	application_id = i;
	return *this;
}

command_option_choice::command_option_choice(const std::string &n, std::variant<std::string, int32_t> v) : name(n), value(v)
{
}

command_option::command_option(command_option_type t, const std::string &n, const std::string &d, bool r) :
	type(t), name(n), description(d), required(r)
{
}

command_option& command_option::add_choice(const command_option_choice &o)
{
	choices.push_back(o);
	return *this;
}

command_option& command_option::add_option(const command_option &o)
{
	options.push_back(o);
	return *this;
}

slashcommand& slashcommand::add_option(const command_option &o)
{
	options.push_back(o);
	return *this;
}

};

