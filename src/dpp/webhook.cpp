#include <dpp/webhook.h>
#include <dpp/discordevents.h>
#include <dpp/discord.h>
#include <nlohmann/json.hpp>

namespace dpp {

using json = nlohmann::json;

webhook::webhook() : managed(), type(w_incoming), guild_id(0), channel_id(0), user_id(0), application_id(0), image_data(nullptr)
{
}

webhook::~webhook() {
	if (image_data) {
		delete image_data;
	}
}

webhook& webhook::fill_from_json(nlohmann::json* j) {
	id = SnowflakeNotNull(j, "id");
	type = Int8NotNull(j, "type");
	channel_id = SnowflakeNotNull(j, "channel_id");
	guild_id = SnowflakeNotNull(j, "guild_id");
	if (j->find("user") != j->end()) {
		json & user = (*j)["user"];
		user_id = SnowflakeNotNull(&user, "id");
	}
	name = StringNotNull(j, "name");
	avatar = StringNotNull(j, "name");
	token = StringNotNull(j, "token");
	application_id = SnowflakeNotNull(j, "application_id");

	return *this;
}

std::string webhook::build_json(bool with_id) const {
	json j;
	if (with_id) {
		j["id"] = std::to_string(id);
	}
	j["name"] = name;
	j["type"] = type;
	if (channel_id)
		j["channel_id"] = channel_id;
	if (guild_id)
		j["guild_id"] = guild_id;
	if (!name.empty())
		j["name"] = name;
	if (image_data)
		j["avatar"] = *image_data;
	if (application_id)
		j["application_id"] = application_id;
	return j.dump();
}

webhook& webhook::load_image(const std::string &image_blob, image_type type) {
	static std::map<image_type, std::string> mimetypes = {
		{ i_gif, "image/gif" },
		{ i_jpg, "image/jpeg" },
		{ i_png, "image/png" }
	};
	if (image_blob.size() > MAX_EMOJI_SIZE) {
		throw std::runtime_error("Webhook icon file exceeds discord limit of 256 kilobytes");
	}
	if (image_data) {
		/* If there's already image data defined, free the old data, to prevent a memory leak */
		delete image_data;
	}
	image_data = new std::string("data:" + mimetypes[type] + ";base64," + base64_encode((unsigned char const*)image_blob.data(), image_blob.length()));

	return *this;
}

};

