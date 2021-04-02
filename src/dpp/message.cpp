#include <dpp/message.h>
#include <nlohmann/json.hpp>
#include <dpp/discordevents.h>

using json = nlohmann::json;

namespace dpp {

embed::~embed() {
}

embed::embed() {
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
			std::optional<embed_image> & curr = image;
			if (s == "image") {
				curr = image;
			} else if (s == "video") {
				curr = video;
			} else if (s == "thumbnail") {
				curr = thumbnail;
			}
			json& fi = (*j)[s];
			dpp::embed_image f;
			f.url = StringNotNull(&fi, "url");
			f.height = StringNotNull(&fi, "height");
			f.width = StringNotNull(&fi, "width");
			f.proxy_url = StringNotNull(&fi, "proxy_url");
			curr = f;
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
			f.is_inline = StringNotNull(&field, "inline");
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

std::string message::build_json(bool with_id) const {
	/* This is the basics. once it works, expand on it. */
	json j({
		{"content", content},
		{"channel_id", channel_id},
		{"tts", tts},
		{"nonce", nonce}
	});
	if (with_id) {
		j["id"] = std::to_string(id);
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

/*
struct embed_image {
        std::string url;
        std::string proxy_url;
        std::string height;
        std::string width;
};

struct embed_field {
        std::string name;
        std::string value;
        std::string is_inline;
};

*/

};
