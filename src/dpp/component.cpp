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
#include <dpp/component.h>
#include <dpp/discordevents.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace dpp {

component::component() : type(static_cast<component_type>(1)), label(""), style(static_cast<component_style>(1)), custom_id(""), disabled(false)
{
}

component::~component() {
}


component& component::fill_from_json(nlohmann::json* j) {

    type = static_cast<component_type>(Int8NotNull(j, "type"));
	if (type == ActionRow) {
        std::vector<component> components;
        for (json sub_component : (*j)["components"]) {
            components.push_back(this->fill_from_json(&sub_component));
        }
    } else {
        label = StringNotNull(j, "label");
        style = static_cast<component_style>(Int8NotNull(j, "style"));
        custom_id = StringNotNull(j, "custom_id");
        disabled = BoolNotNull(j, "disabled");
    }
	return *this;
}

std::string component::build_json() const {
	json j;
	if (type == component_type::ActionRow) {
        j["type"] = 1;
        json new_components;
        for (component new_component : components) {
            new_components.push_back(new_component.build_json());
        }
        j["components"] = new_components;
    } else {
        j["type"] = 2;
        j["label"] = label;
        j["style"] = int(style);
        j["custom_id"] = custom_id;
        j["disabled"] = disabled;
    }
	return j.dump();
}

};
