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
#include <dpp/message.h>
#include <dpp/user.h>
#include <dpp/channel.h>
#include <dpp/guild.h>
#include <dpp/cache.h>
#include <dpp/nlohmann/json.hpp>
#include <dpp/discordevents.h>
#include <dpp/stringops.h>

using json = nlohmann::json;

namespace dpp {

component::component() : type(static_cast<component_type>(1)), label(""), style(static_cast<component_style>(1)), custom_id(""), disabled(false), min_values(-1), max_values(-1)
{
	emoji.animated = false;
	emoji.id = 0;
	emoji.name = "";
}


component& component::fill_from_json(nlohmann::json* j) {
	type = static_cast<component_type>(Int8NotNull(j, "type"));
	if (type == cot_action_row) {
		components;
		for (json sub_component : (*j)["components"]) {
			dpp::component new_component;
			new_component.fill_from_json(&sub_component);
			components.push_back(new_component);
		}
	} else if (type == cot_button) {
		label = StringNotNull(j, "label");
		style = static_cast<component_style>(Int8NotNull(j, "style"));
		custom_id = StringNotNull(j, "custom_id");
		disabled = BoolNotNull(j, "disabled");
	} else if (type == cot_selectmenu) {
		label = "";
		custom_id = StringNotNull(j, "custom_id");
		disabled = BoolNotNull(j, "disabled");
	}
	return *this;
}

component& component::add_component(const component& c)
{
	set_type(cot_action_row);
	components.push_back(c);
	return *this;
}

component& component::set_type(component_type ct)
{
	type = ct;
	return *this;
}

component& component::set_label(const std::string &l)
{
	if (type == cot_action_row) {
		set_type(cot_button);
	}
	label = utility::utf8substr(l, 0, 80);
	return *this;
}

component& component::set_style(component_style cs)
{
	set_type(cot_button);
	style = cs;
	return *this;
}

component& component::set_url(const std::string& u)
{
	set_type(cot_button);
	set_style(cos_link);
	url = utility::utf8substr(u, 0, 512);
	return *this;
}

component& component::set_id(const std::string &id)
{
	if (type == cot_action_row) {
		set_type(cot_button);
	}
	custom_id = utility::utf8substr(id, 0, 100);
	return *this;
}

component& component::set_disabled(bool disable)
{
	if (type == cot_action_row) {
		set_type(cot_button);
	}
	disabled = disable;
	return *this;
}

component& component::set_emoji(const std::string& name, dpp::snowflake id, bool animated)
{
	if (type == cot_action_row) {
		set_type(cot_button);
	}
	this->emoji.id = id;
	this->emoji.name = name;
	this->emoji.animated = animated;
	return *this;
}

std::string component::build_json() const {
	json j;
	if (type == component_type::cot_action_row) {
		j["type"] = 1;
		json new_components;
		for (component new_component : components) {
			new_components.push_back(new_component.build_json());
		}
		j["components"] = new_components;
	} else if (type == component_type::cot_button) {
		j["type"] = 2;
		j["label"] = label;
		j["style"] = int(style);
		j["custom_id"] = custom_id;
		j["disabled"] = disabled;
	} else if (type == component_type::cot_selectmenu) {
		j["type"] = 3;
		j["custom_id"] = custom_id;
		//j["disabled"] = disabled;
		if (!placeholder.empty()) {
			j["placeholder"] = placeholder;
		}
		if (min_values >= 0) {
			j["min_values"] = min_values;
		}
		if (max_values >= 0) {
			j["max_values"] = max_values;
		}
		j["options"] = json::array();
		for (auto opt : options) {
			json o;
			if (!opt.description.empty()) {
				o["description"] = opt.description;
			}
			if (!opt.label.empty()) {
				o["label"] = opt.label;
			}
			if (!opt.value.empty()) {
				o["value"] = opt.value;
			}
			if (opt.is_default) {
				o["default"] = true;
			}
			if (!opt.emoji.name.empty()) {
				o["emoji"] = json::object();
				o["emoji"]["name"] = opt.emoji.name;
				if (opt.emoji.id) {
					o["emoji"]["id"] = std::to_string(opt.emoji.id);
				}
			}
			j["options"].push_back(o);
		}
	}
	return j.dump();
}

select_option::select_option() : is_default(false) {
}

select_option::select_option(const std::string &_label, const std::string &_value, const std::string &_description) : is_default(false), label(_label), value(_value), description(_description) {
}

select_option& select_option::set_label(const std::string &l) {
	label = dpp::utility::utf8substr(l, 0, 100);
	return *this;
}

select_option& select_option::set_default(bool def) {
	is_default = def;
	return *this;
}

select_option& select_option::set_value(const std::string &v) {
	value = dpp::utility::utf8substr(v, 0, 100);
	return *this;
}

select_option& select_option::set_description(const std::string &d) {
	description = dpp::utility::utf8substr(d, 0, 100);
	return *this;
}

select_option& select_option::set_emoji(const std::string &n, dpp::snowflake id, bool animated) {
	emoji.name = n;
	emoji.id = id;
	emoji.animated = animated;
	return *this;
}

select_option& select_option::set_animated(bool anim) {
	emoji.animated = anim;
	return *this;
}


component& component::set_placeholder(const std::string &_placeholder) {
	placeholder = dpp::utility::utf8substr(_placeholder, 0, 100);
	return *this;
}

component& component::set_min_values(uint32_t _min_values) {
	min_values = _min_values;
	return *this;
}

component& component::set_max_values(uint32_t _max_values) {
	max_values = _max_values;
	return *this;
}

component& component::add_select_option(const select_option &option) {
	if (options.size() <= 25) {
		options.push_back(option);
	}
	return *this;
}

embed::~embed() {
}

embed::embed() : timestamp(0), color(0) {
}

message::message() : id(0), channel_id(0), guild_id(0), author(nullptr), sent(0), edited(0), flags(0),
	type(mt_default), tts(false), mention_everyone(false), pinned(false), webhook_id(0)
{
	message_reference.channel_id = 0;
	message_reference.guild_id = 0;
	message_reference.message_id = 0;
	message_reference.fail_if_not_exists = false;
	interaction.id = 0;
	interaction.type = interaction_type::it_ping;
	interaction.usr.id = 0;
	allowed_mentions.parse_users = false;
	allowed_mentions.parse_everyone = false;
	allowed_mentions.parse_roles = false;
	/* The documentation for discord is INCORRECT. This defaults to true, and must be set to false.
	 * The default ctor reflects this.
	 */
	allowed_mentions.replied_user = true;

}

message& message::set_reference(snowflake _message_id, snowflake _guild_id, snowflake _channel_id, bool fail_if_not_exists) {
	message_reference.channel_id = _channel_id;
	message_reference.guild_id = _guild_id;
	message_reference.message_id = _message_id;
	message_reference.fail_if_not_exists = fail_if_not_exists;
	return *this;
}

message& message::set_allowed_mentions(bool _parse_users, bool _parse_roles, bool _parse_everyone, bool _replied_user, const std::vector<snowflake> &users, const std::vector<snowflake> &roles) {
	allowed_mentions.parse_users = _parse_users;
	allowed_mentions.parse_everyone = _parse_everyone;
	allowed_mentions.parse_roles = _parse_roles;
	allowed_mentions.replied_user = _replied_user;
	allowed_mentions.users = users;
	allowed_mentions.roles = roles;
	return *this;
}

message::message(snowflake _channel_id, const std::string &_content, message_type t) : message() {
	channel_id = _channel_id;
	content = utility::utf8substr(_content, 0, 2000);
	type = t;
}

message& message::add_component(const component& c)
{
	components.push_back(c);
	return *this;
}

message& message::add_embed(const embed& e)
{
	embeds.push_back(e);
	return *this;
}

message& message::set_flags(uint8_t f)
{
	flags = f;
	return *this;
}

message& message::set_type(message_type t)
{
	type = t;
	return *this;
}

message& message::set_filename(const std::string &fn)
{
	filename = fn;
	return *this;
}

message& message::set_file_content(const std::string &fc)
{
	filecontent = fc;
	return *this;
}

message& message::set_content(const std::string &c)
{
	content = utility::utf8substr(c, 0, 2000);
	return *this;
}

message::message(const std::string &_content, message_type t) : message() {
	content = utility::utf8substr(_content, 0, 2000);
	type = t;
}

message::message(snowflake _channel_id, const embed& _embed) : message() {
	channel_id = _channel_id;
	embeds.push_back(_embed);
}

embed::embed(json* j) : embed() {
	title = StringNotNull(j, "title");
	type = StringNotNull(j, "type");
	description = StringNotNull(j, "description");
	url = StringNotNull(j, "url");
	timestamp = TimestampNotNull(j, "timestamp");
	color = Int32NotNull(j, "color");
	if (j->find("footer") != j->end()) {
		dpp::embed_footer f;
		json& fj = (*j)["footer"];
		f.text = StringNotNull(&fj, "text");
		f.icon_url = StringNotNull(&fj, "icon_url");
		f.proxy_url = StringNotNull(&fj, "proxy_url");
		footer = f;
	}
	std::vector<std::string> type_list = { "image", "video", "thumbnail" };
	for (auto& s : type_list) {
		if (j->find(s) != j->end()) {
			embed_image curr;
			json& fi = (*j)[s];
			curr.url = StringNotNull(&fi, "url");
			curr.height = StringNotNull(&fi, "height");
			curr.width = StringNotNull(&fi, "width");
			curr.proxy_url = StringNotNull(&fi, "proxy_url");
			if (s == "image") {
				image = curr;
			} else if (s == "video") {
				video = curr;
			} else if (s == "thumbnail") {
				thumbnail = curr;
			}
		}
	}
	if (j->find("provider") != j->end()) {
		json &p = (*j)["provider"];
		dpp::embed_provider pr;
		pr.name = StringNotNull(&p, "name");
		pr.url = StringNotNull(&p, "url");
		provider = pr;
	}
	if (j->find("author") != j->end()) {
		json &a = (*j)["author"];
		dpp::embed_author au;
		au.name = StringNotNull(&a, "name");
		au.url = StringNotNull(&a, "url");
		au.icon_url = StringNotNull(&a, "icon_url");
		au.proxy_icon_url = StringNotNull(&a, "proxy_icon_url");
		author = au;
	}
	if (j->find("fields") != j->end()) {
		json &fl = (*j)["fields"];
		for (auto & field : fl) {
			embed_field f;
			f.name = StringNotNull(&field, "name");
			f.value = StringNotNull(&field, "value");
			f.is_inline = BoolNotNull(&field, "inline");
			fields.push_back(f);
		}
	}
}

embed& embed::add_field(const std::string& name, const std::string &value, bool is_inline) {
	if (fields.size() < 25) {
		embed_field f;
		f.name = utility::utf8substr(name, 0, 256);
		f.value = utility::utf8substr(value, 0, 1024);
		f.is_inline = is_inline;
		fields.push_back(f);
	}
	return *this;
}

embed& embed::set_author(const embed_author& a)
{
	author = a;
	return *this;
}

embed& embed::set_author(const std::string& name, const std::string& url, const std::string& icon_url) {
	dpp::embed_author a;
	a.name = utility::utf8substr(name, 0, 256);
	a.url = url;
	a.icon_url = icon_url;
	author = a;
	return *this;
}

embed& embed::set_footer(const embed_footer& f) {
	footer = f;
	return *this;
}

embed& embed::set_provider(const std::string& name, const std::string& url) {
	dpp::embed_provider p;
	p.name = utility::utf8substr(name, 0, 256);
	p.url = url;
	provider = p;
	return *this;
}

embed& embed::set_image(const std::string& url) {
	dpp::embed_image i;
	i.url = url;
	image = i;
	return *this;
}

embed& embed::set_video(const std::string& url) {
	dpp::embed_image v;
	v.url = url;
	video = v;
	return *this;
}

embed& embed::set_thumbnail(const std::string& url) {
	dpp::embed_image t;
	t.url = url;
	thumbnail = t;
	return *this;
}

embed& embed::set_title(const std::string &text) {
	title = utility::utf8substr(text, 0, 256);
	return *this;
}

embed& embed::set_description(const std::string &text) {
	description = utility::utf8substr(text, 0, 4096);
	return *this;
}

embed& embed::set_color(uint32_t col) {
	// Mask off alpha, as discord doesn't use it
	color = col & 0x00FFFFFF;
	return *this;
}

embed& embed::set_url(const std::string &u) {
	url = u;
	return *this;
}

embed_footer& embed_footer::set_text(const std::string& t){
	text = t; 
	return *this;     
}

embed_footer& embed_footer::set_icon(const std::string& i){     
	icon_url = i;
	return *this;           
}                                                                                  

embed_footer& embed_footer::set_proxy(const std::string& p){     
	proxy_url = p;          
	return *this;
}

reaction::reaction() {
	count = 0;
	me = false;
	emoji_id = 0;
}

reaction::reaction(json* j) {
	count = (*j)["count"];
	me = (*j)["me"];
	json emoji = (*j)["emoji"];
	emoji_id = SnowflakeNotNull(&emoji, "id");
	emoji_name = StringNotNull(&emoji, "name");
}

attachment::attachment() 
	: id(0)
	, size(0)
	, width(0)
	, height(0)
	, ephemeral(false)
{
}

attachment::attachment(json *j) : attachment() {
	this->id = SnowflakeNotNull(j, "id");
	this->size = (*j)["size"];
	this->filename = (*j)["filename"];
	this->url = (*j)["url"];
	this->proxy_url = (*j)["proxy_url"];
	this->width = Int32NotNull(j, "width");
	this->height = Int32NotNull(j, "height");
	this->content_type = StringNotNull(j, "content_type");
	this->ephemeral = BoolNotNull(j, "ephemeral");
}

std::string message::build_json(bool with_id, bool is_interaction_response) const {
	/* This is the basics. once it works, expand on it. */
	json j({
		{"channel_id", channel_id},
		{"tts", tts},
		{"nonce", nonce},
		{"flags", flags},
		{"type", type}
	});

	if (with_id) {
		j["id"] = std::to_string(id);
	}

	if (!content.empty()) {
		j["content"] = content;
	}

    if(author != nullptr) {
        /* Used for webhooks */
        j["username"] = author->username;
    }

	/* Populate message reference */
	if (message_reference.channel_id || message_reference.guild_id || message_reference.message_id) {
		j["message_reference"] = json::object();
		if (message_reference.channel_id) {
			j["message_reference"]["channel_id"] = std::to_string(message_reference.channel_id);
		}
		if (message_reference.guild_id) {
			j["message_reference"]["guild_id"] = std::to_string(message_reference.guild_id);
		}
		if (message_reference.message_id) {
			j["message_reference"]["message_id"] = std::to_string(message_reference.message_id);
		}
		j["message_reference"]["fail_if_not_exists"] = message_reference.fail_if_not_exists;
	}

	j["allowed_mentions"] = json::object();
	j["allowed_mentions"]["parse"] = json::array();
	if (allowed_mentions.parse_everyone || allowed_mentions.parse_roles || allowed_mentions.parse_users || !allowed_mentions.replied_user || allowed_mentions.users.size() || allowed_mentions.roles.size()) {
		if (allowed_mentions.parse_everyone) {
			j["allowed_mentions"]["parse"].push_back("everyone");
		}
		if (allowed_mentions.parse_roles) {
			j["allowed_mentions"]["parse"].push_back("roles");
		}
		if (allowed_mentions.parse_users) {
			j["allowed_mentions"]["parse"].push_back("users");
		}
		if (!allowed_mentions.replied_user) {
			j["allowed_mentions"]["replied_user"] = false;
		}
		if (allowed_mentions.users.size()) {
			j["allowed_mentions"]["users"] = json::array();
			for (auto& user : allowed_mentions.users) {
				j["allowed_mentions"]["users"].push_back(std::to_string(user));
			}
		}
		if (allowed_mentions.roles.size()) {
			j["allowed_mentions"]["roles"] = json::array();
			for (auto& role : allowed_mentions.roles) {
				j["allowed_mentions"]["roles"].push_back(std::to_string(role));
			}
		}
	}


	if (components.size()) {
		j["components"] = json::array();
	}
	for (auto & component : components) {
		json n;
		n["type"] = cot_action_row;
		n["components"] = {};
		json sn;
		for (auto & subcomponent  : component.components) {
			if (subcomponent.type == cot_button) {
				sn["type"] = subcomponent.type;
				sn["label"] = subcomponent.label;
				sn["style"] = int(subcomponent.style);
				if (subcomponent.type == cot_button && subcomponent.style != cos_link && !subcomponent.custom_id.empty()) {
					/* Links cannot have a custom id */
					sn["custom_id"] = subcomponent.custom_id;
				}
				if (subcomponent.type == cot_button && subcomponent.style == cos_link && !subcomponent.url.empty()) {
					sn["url"] = subcomponent.url;
				}
				sn["disabled"] = subcomponent.disabled;

				if (subcomponent.emoji.id || !subcomponent.emoji.name.empty()) {
					sn["emoji"] = {};
					sn["emoji"]["animated"] = subcomponent.emoji.animated;
				}
				if (subcomponent.emoji.id) {
					sn["emoji"]["id"] = std::to_string(subcomponent.emoji.id);
				}
				if (!subcomponent.emoji.name.empty()) {
					sn["emoji"]["name"] = subcomponent.emoji.name;
				}
			} else if (subcomponent.type == cot_selectmenu) {

				sn["type"] = subcomponent.type;
				sn["custom_id"] = subcomponent.custom_id;
				//sn["disabled"] = subcomponent.disabled;
				if (!subcomponent.placeholder.empty()) {
					sn["placeholder"] = subcomponent.placeholder;
				}
				if (subcomponent.min_values >= 0) {
					sn["min_values"] = subcomponent.min_values;
				}
				if (subcomponent.max_values >= 0) {
					sn["max_values"] = subcomponent.max_values;
				}
				sn["options"] = json::array();
				for (auto opt : subcomponent.options) {
					json o;
					if (!opt.description.empty()) {
						o["description"] = opt.description;
					}
					if (!opt.label.empty()) {
						o["label"] = opt.label;
					}
					if (!opt.value.empty()) {
						o["value"] = opt.value;
					}
					if (opt.is_default) {
						o["default"] = true;
					}
					if (!opt.emoji.name.empty()) {
						o["emoji"] = json::object();
						o["emoji"]["name"] = opt.emoji.name;
						if (opt.emoji.id) {
							o["emoji"]["id"] = std::to_string(opt.emoji.id);
						}
						if (opt.emoji.animated) {
							o["emoji"]["animated"] = true;
						}
					}
					sn["options"].push_back(o);
				}
			}

			n["components"].push_back(sn);
		}
		j["components"].push_back(n);
	}
	if (embeds.size()) {
		j["embeds"] = json::array();

		for (auto& embed : embeds) {
			json e;
			if (!embed.description.empty())
				e["description"] = embed.description;
			if (!embed.title.empty())
				e["title"] = embed.title;
			if (!embed.url.empty())
				e["url"] = embed.url;
			e["color"] = embed.color;
			if (embed.footer.has_value()) {
				e["footer"]["text"] = embed.footer->text;
				e["footer"]["icon_url"] = embed.footer->icon_url;
			}
			if (embed.image.has_value()) {
				e["image"]["url"] = embed.image->url;
			}
			if (embed.thumbnail.has_value()) {
				e["thumbnail"]["url"] = embed.thumbnail->url;
			}
			if (embed.author.has_value()) {
				e["author"]["name"] = embed.author->name;
				e["author"]["url"] = embed.author->url;
				e["author"]["icon_url"] = embed.author->icon_url;
			}
			if (embed.fields.size()) {
				e["fields"] = json();
				for (auto& field : embed.fields) {
					json f({ {"name", field.name}, {"value", field.value}, {"inline", field.is_inline} });
					e["fields"].push_back(f);
				}
			}
			if (embed.timestamp != 0) {
				std::ostringstream ss;
				struct tm t;
			
			#ifdef _WIN32
				gmtime_s(&t, &embed.timestamp);
			#else
				gmtime_r(&embed.timestamp, &t);
			#endif
				
				ss << std::put_time(&t, "%FT%TZ");
				e["timestamp"] = ss.str();
			}

				j["embeds"].push_back(e);
		}
	}
	return j.dump();
}

bool message::is_crossposted() const {
	return flags & m_crossposted;
}

bool message::is_crosspost() const {
	return flags & m_is_crosspost;

}

bool message::suppress_embeds() const {
	return flags & m_suppress_embeds;
}

bool message::is_source_message_deleted() const {
	return flags & m_source_message_deleted;
}

bool message::is_urgent() const {
	return flags & m_urgent;
}

bool message::is_ephemeral() const {
	return flags & m_ephemeral;
}

bool message::is_loading() const {
	return flags & m_loading;
}

message::~message() {
}


message& message::fill_from_json(json* d, cache_policy_t cp) {
	this->id = SnowflakeNotNull(d, "id");
	this->channel_id = SnowflakeNotNull(d, "channel_id");
	this->guild_id = SnowflakeNotNull(d, "guild_id");
	/* We didn't get a guild id. See if we can find one in the channel */
	if (guild_id == 0 && channel_id != 0) {
		dpp::channel* c = dpp::find_channel(this->channel_id);
		if (c) {
			this->guild_id = c->guild_id;
		}
	}
	this->flags = Int8NotNull(d, "flags");
	this->type = Int8NotNull(d, "type");
	this->author = nullptr;
	user* authoruser = nullptr;
	/* May be null, if its null cache it from the partial */
	if (d->find("author") != d->end()) {
		json &j_author = (*d)["author"];
		if (cp.user_policy == dpp::cp_none) {
			/* User caching off! Allocate a temp user to be deleted in destructor */
			authoruser = &self_author;
			this->author = &self_author;
			self_author.fill_from_json(&j_author);
		} else {
			/* User caching on - aggressive or lazy - create a cached user entry */
			authoruser = find_user(SnowflakeNotNull(&j_author, "id"));
			if (!authoruser) {
				/* User does not exist yet, cache the partial as a user record */
				authoruser = new user();
				authoruser->fill_from_json(&j_author);
				get_user_cache()->store(authoruser);
			}
			this->author = authoruser;
		}
	}
	if (d->find("interaction") != d->end()) {
		json& inter = (*d)["interaction"];
		interaction.id = SnowflakeNotNull(&inter, "id");
		interaction.name = StringNotNull(&inter, "name");
		interaction.type = Int8NotNull(&inter, "type");
		if (inter.contains("user") && !inter["user"].is_null()) from_json(inter["user"], interaction.usr);
	}
	if (d->find("sticker_items") != d->end()) {
		json &sub = (*d)["sticker_items"];
		for (auto & sticker_raw : sub) {
			stickers.push_back(dpp::sticker().fill_from_json(&sticker_raw));
		}
	}
	if (d->find("mentions") != d->end()) {
		json &sub = (*d)["mentions"];
		for (auto & m : sub) {
			mentions.push_back(SnowflakeNotNull(&m, "id"));
		}
	}
	if (d->find("mention_roles") != d->end()) {
		json &sub = (*d)["mention_roles"];
		for (auto & m : sub) {
			mention_roles.push_back(from_string<snowflake>(m, std::dec));
		}
	}
	if (d->find("mention_channels") != d->end()) {
		json &sub = (*d)["mention_channels"];
		for (auto & m : sub) {
			mention_channels.push_back(SnowflakeNotNull(&m, "id"));
		}
	}
	/* Fill in member record, cache uncached ones */
	guild* g = find_guild(this->guild_id);
	this->member = {};
	if (g && d->find("member") != d->end()) {
		json& mi = (*d)["member"];
		snowflake uid = SnowflakeNotNull(&(mi["user"]), "id");
		if (!uid && authoruser) {
			uid = authoruser->id;
		}
		if (cp.user_policy == dpp::cp_none) {
			/* User caching off! Just fill in directly but dont store member to guild */
			this->member.fill_from_json(&mi, g->id, uid);
		} else {
			/* User caching on, lazy or aggressive - cache the member information */
			auto thismember = g->members.find(uid);
			if (thismember == g->members.end()) {
				if (uid != 0 && authoruser) {
					guild_member gm;
					gm.fill_from_json(&mi, g->id, uid);
					g->members[authoruser->id] = gm;
					this->member = gm;
				}
			} else {
				/* Update roles etc */
				this->member = thismember->second;
				if (authoruser) {
					this->member.fill_from_json(&mi, g->id, authoruser->id);
					g->members[authoruser->id] = this->member;
				}
			}
		}
	}
	if (d->find("embeds") != d->end()) {
		json & el = (*d)["embeds"];
		for (auto& e : el) {
			this->embeds.push_back(embed(&e));
		}
	}
	this->content = StringNotNull(d, "content");
	this->sent = TimestampNotNull(d, "timestamp");
	this->edited = TimestampNotNull(d, "edited_timestamp");
	this->tts = BoolNotNull(d, "tts");
	this->mention_everyone = BoolNotNull(d, "mention_everyone");
	if (d->find("reactions") != d->end()) {
		json & el = (*d)["reactions"];
		for (auto& e : el) {
			this->reactions.push_back(reaction(&e));
		}
	}
	if (((*d)["nonce"]).is_string()) {
		this->nonce = StringNotNull(d, "nonce");
	} else {
		this->nonce = std::to_string(SnowflakeNotNull(d, "nonce"));
	}
	this->pinned = BoolNotNull(d, "pinned");
	this->webhook_id = SnowflakeNotNull(d, "webhook_id");
	for (auto& e : (*d)["attachments"]) {
		this->attachments.push_back(attachment(&e));
	}
	return *this;
}

sticker::sticker() : id(0), pack_id(0), guild_id(0), type(st_standard), format_type(sf_png), available(true), sort_value(0) {
}

sticker& sticker::fill_from_json(nlohmann::json* j) {
	this->id = SnowflakeNotNull(j, "id");
	this->pack_id = SnowflakeNotNull(j, "pack_id");
	this->name = StringNotNull(j, "name");
	this->description = StringNotNull(j, "description");
	this->tags = StringNotNull(j, "tags");
	this->asset = StringNotNull(j, "asset");
	this->guild_id = SnowflakeNotNull(j, "guild_id");
	this->type = static_cast<sticker_type>(Int8NotNull(j, "type"));
	this->format_type = static_cast<sticker_format>(Int8NotNull(j, "format_type"));
	this->available = BoolNotNull(j, "available");
	this->sort_value = Int8NotNull(j, "sort_value");
	if (j->find("user") != j->end()) {
		sticker_user.fill_from_json(&((*j)["user"]));
	}

	return *this;
}

std::string sticker::build_json(bool with_id) const {
	json j;

	if (with_id) {
		j["id"] = std::to_string(this->id);
	}
	j["pack_id"] = std::to_string(this->pack_id);
	if (this->guild_id) {
		j["guild_id"] = std::to_string(this->guild_id);
	}
	j["name"] = this->name;
	j["description"] = this->description;
	if (!this->tags.empty()) {
		j["tags"] = this->tags;
	}
	if (!this->asset.empty()) {
		j["asset"] = this->asset;
	}
	j["type"] = this->type;
	j["format_type"] = this->format_type;
	j["available"] = this->available;
	j["sort_value"] = this->sort_value;

	return j.dump();
}

sticker_pack::sticker_pack() : id(0), sku_id(0), cover_sticker_id(0), banner_asset_id(0) {
}

sticker_pack& sticker_pack::fill_from_json(nlohmann::json* j) {
	this->id = SnowflakeNotNull(j, "id");
	this->sku_id = SnowflakeNotNull(j, "sku_id");
	this->cover_sticker_id = SnowflakeNotNull(j, "cover_sticker_id");
	this->banner_asset_id = SnowflakeNotNull(j, "banner_asset_id");
	this->name = StringNotNull(j, "name");
	this->description = StringNotNull(j, "description");
	if (j->find("stickers") != j->end()) {
		json & sl = (*j)["stickers"];
		for (auto& s : sl) {
			this->stickers[SnowflakeNotNull(&s, "id")] = sticker().fill_from_json(&s);
		}
	}
	return *this;
}

std::string sticker_pack::build_json(bool with_id) const {
	json j;
	if (with_id) {
		j["id"] = std::to_string(this->id);
	}
	if (sku_id) {
		j["sku_id"] = std::to_string(sku_id);
	}
	if (cover_sticker_id) {
		j["cover_sticker_id"] = std::to_string(cover_sticker_id);
	}
	if (banner_asset_id) {
		j["banner_asset_id"] = std::to_string(banner_asset_id);
	}
	j["name"] = name;
	j["description"] = description;
	j["stickers"] = json::array();
	for (auto& s : stickers) {
		j["stickers"].push_back(json::parse(s.second.build_json(with_id)));
	}
	return j.dump();
}

sticker& sticker::set_filename(const std::string &fn) {
	filename = fn;
	return *this;
}

sticker& sticker::set_file_content(const std::string &fc) {
	filecontent = fc;
	return *this;
}


};
