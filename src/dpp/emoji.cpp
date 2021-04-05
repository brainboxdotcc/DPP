#include <dpp/emoji.h>
#include <dpp/discordevents.h>
#include <dpp/discord.h>
#include <nlohmann/json.hpp>

namespace dpp {

using json = nlohmann::json;

emoji::emoji() : managed(), user_id(0), flags(0), image_data(nullptr)
{
}

emoji::~emoji() {
	if (image_data) {
		delete image_data;
	}
}

emoji& emoji::fill_from_json(nlohmann::json* j) {
	id = SnowflakeNotNull(j, "id");
	name = StringNotNull(j, "name");
	if (j->find("user") != j->end()) {
		json & user = (*j)["user"];
		user_id = SnowflakeNotNull(&user, "id");
	}
	if (BoolNotNull(j, "require_colons"))
		flags |= e_require_colons;
	if (BoolNotNull(j, "managed"))
		flags |= e_managed;
	if (BoolNotNull(j, "animated"))
		flags |= e_animated;
	if (BoolNotNull(j, "available"))
		flags |= e_available;
	return *this;
}

std::string emoji::build_json(bool with_id) const {
	json j;
	if (with_id) {
		j["id"] = std::to_string(id);
	}
	j["name"] = name;
	if (image_data) {
		j["image"] = *image_data;
	}
	return j.dump();
}

bool emoji::requires_colons() const {
	return flags & e_require_colons;
}

bool emoji::is_managed() const {
	return flags & e_managed;
}

bool emoji::is_animated() const {
	return flags & e_animated;
}

bool emoji::is_available() const {
	return flags & e_available;
}

std::string base64_encode(unsigned char const* buf, unsigned int buffer_length) {

	static const char to_base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	size_t ret_size = buffer_length + 2;

	ret_size = 4 * ret_size / 3;

	std::string ret;
	ret.reserve(ret_size);

	for (unsigned int i=0; i<ret_size/4; ++i)
	{
		size_t index = i*3;
		unsigned char b3[3];
		b3[0] = buf[index+0];
		b3[1] = buf[index+1];
		b3[2] = buf[index+2];

		ret.push_back(to_base64[ ((b3[0] & 0xfc) >> 2) ]);
		ret.push_back(to_base64[ ((b3[0] & 0x03) << 4) + ((b3[1] & 0xf0) >> 4) ]);
		ret.push_back(to_base64[ ((b3[1] & 0x0f) << 2) + ((b3[2] & 0xc0) >> 6) ]);
		ret.push_back(to_base64[ ((b3[2] & 0x3f)) ]);
	}

	return ret;
}

emoji& emoji::load_image(const std::string &image_blob, image_type type) {
	static std::map<image_type, std::string> mimetypes = {
		{ i_gif, "image/gif" },
		{ i_jpg, "image/jpeg" },
		{ i_png, "image/png" }
	};
	if (image_blob.size() > MAX_EMOJI_SIZE) {
		throw std::runtime_error("Emoji file exceeds discord limit of 256 kilobytes");
	}
	if (image_data) {
		/* If there's already image data defined, free the old data, to prevent a memory leak */
		delete image_data;
	}
	image_data = new std::string("data:" + mimetypes[type] + ";base64," + base64_encode((unsigned char const*)image_blob.data(), image_blob.length()));

	return *this;
}

};

