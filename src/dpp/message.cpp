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
#include <nlohmann/json.hpp>
#include <dpp/discordevents.h>
#include <dpp/stringops.h>

using json = nlohmann::json;

namespace dpp {

component::component() : type(static_cast<component_type>(1)), label(""), style(static_cast<component_style>(1)), custom_id(""), disabled(false)
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
	set_type(cot_button);
	label = l.substr(0, 80);
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
	url = u.substr(0, 512);
	return *this;
}

component& component::set_id(const std::string &id)
{
	set_type(cot_button);
	custom_id = id.substr(0, 100);
	return *this;
}

component& component::set_disabled(bool disable)
{
	set_type(cot_button);
	disabled = disable;
	return *this;
}

component& component::set_emoji(const std::string& name, dpp::snowflake id, bool animated)
{
	set_type(cot_button);
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
	} else {
		j["type"] = 2;
		j["label"] = label;
		j["style"] = int(style);
		j["custom_id"] = custom_id;
		j["disabled"] = disabled;
	}
	return j.dump();
}

embed::~embed() {
}

embed::embed() {
}

message::message() : id(0), channel_id(0), guild_id(0), author(nullptr), member(nullptr), sent(0), edited(0), flags(0),
	type(mt_default), tts(false), mention_everyone(false), pinned(false), webhook_id(0)
{

}

message::message(snowflake _channel_id, const std::string &_content, message_type t) : message() {
	channel_id = _channel_id;
	content = _content;
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

message::message(const std::string &_content, message_type t) : message() {
	content = _content;
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
	embed_field f;
	f.name = name;
	f.value = value;
	f.is_inline = is_inline;
	fields.push_back(f);
	return *this;
}

embed& embed::set_author(const std::string& name, const std::string& url, const std::string& icon_url) {
	dpp::embed_author a;
	a.name = name;
	a.url = url;
	a.icon_url = icon_url;
	author = a;
	return *this;
}

embed& embed::set_provider(const std::string& name, const std::string& url) {
	dpp::embed_provider p;
	p.name = name;
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
	title = text;
	return *this;
}

embed& embed::set_description(const std::string &text) {
	description = text;
	return *this;
}

embed& embed::set_color(uint32_t col) {
	color = col;
	return *this;
}

embed& embed::set_url(const std::string &u) {
	url = u;
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

attachment::attachment() {
	id = 0;
	size = 0;
	width = 0;
	height = 0;
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
}

std::string message::build_json(bool with_id) const {
	/* This is the basics. once it works, expand on it. */
	json j({
		{"content", content},
		{"channel_id", channel_id},
		{"tts", tts},
		{"nonce", nonce},
		{"flags", flags},
		{"type", type}
	});
	if (with_id) {
		j["id"] = std::to_string(id);
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

			n["components"].push_back(sn);
		}
		j["components"].push_back(n);
	}
	if (embeds.size()) {
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

			/* Sending embeds only accepts the first entry */
			j["embed"] = e;
			break;
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

bool message::supress_embeds() const {
	return flags & m_supress_embeds;
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


message& message::fill_from_json(json* d) {
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
		json &author = (*d)["author"];
		authoruser = find_user(SnowflakeNotNull(&author, "id"));
		if (!authoruser) {
			/* User does not exist yet, cache the partial as a user record */
			authoruser = new user();
			authoruser->fill_from_json(&author);
			get_user_cache()->store(authoruser);
		}
		this->author = authoruser;
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
	this->member = nullptr;
	if (g && d->find("member") != d->end()) {
		json& mi = (*d)["member"];
		snowflake uid = SnowflakeNotNull(&(mi["user"]), "id");
		if (!uid && authoruser) {
			uid = authoruser->id;
		}
		auto thismember = g->members.find(uid);
		if (thismember == g->members.end()) {
			if (uid != 0 && authoruser) {
				guild_member* gm = new guild_member();
				gm->fill_from_json(&mi, g, authoruser);
				g->members.insert(std::make_pair(authoruser->id, gm));
				this->member = gm;
			}
		} else {
			/* Update roles etc */
			this->member = thismember->second;
			if (authoruser) {
				this->member->fill_from_json(&mi, g, authoruser);
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

};

