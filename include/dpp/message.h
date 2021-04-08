#pragma once

#include <dpp/discord.h>
#include <optional>
#include <dpp/json_fwd.hpp>

namespace dpp {

/** A footer in a dpp::embed */
struct embed_footer {
	/** Footer text */
	std::string text;
	/** Footer icon url */
	std::string icon_url;
	/** Proxied icon url */
	std::string proxy_url;
};

/** An video, image or thumbnail in a dpp::embed */
struct embed_image {
	/** URL to image or video */
	std::string url;
	/** Proxied image url */
	std::string proxy_url;
	/** Height (calculated by discord) */
	std::string height;
	/** Width (calculated by discord) */
	std::string width;
};

/** Embed provider in a dpp::embed. Received from discord but cannot be sent
 */
struct embed_provider {
	/** Provider name */
	std::string name;
	/** Provider URL */
	std::string url;
};

/** Author within a dpp::embed object */
struct embed_author {
	/** Author name */
	std::string name;
	/** Author url */
	std::string url;
	/** Author icon url */
	std::string icon_url;
	/** Proxied icon url */
	std::string proxy_icon_url;
};

/** A dpp::embed may contain zero or more fields */
struct embed_field {
	/** Name of field */
	std::string name;
	/** Value of field (max length 1000) */
	std::string value;
	/** True if the field is to be displayed inline */
	std::string is_inline;
};

/** A rich embed for display within a dpp::message */
struct embed {
	/** Optional: title of embed */
	std::string			title;
	/** Optional: type of embed (always "rich" for webhook embeds) */
	std::string			type;
	/** Optional: description of embed */
	std::string			description;
	/** Optional: url of embed */
	std::string			url;
	/** Optional: timestamp of embed content */
	time_t				timestamp;
	/** Optional: color code of the embed */
	uint32_t			color;
	/** Optional: footer information */
	std::optional<embed_footer>	footer;
	/** Optional: image information */
	std::optional<embed_image>	image;
	/** Optional: thumbnail information */
	std::optional<embed_image>	thumbnail;
	/** Optional: video information (can't send these) */
	std::optional<embed_image>	video;
	/** Optional: provider information (can't send these) */
	std::optional<embed_provider>	provider;
	/** Optional: author information */
	std::optional<embed_author>	author;
	/** Optional: fields information */
	std::vector<embed_field>	fields;

	/** Constructor */
	embed();

	/** Constructor to build embed from json object 
	 * @param j JSON to read content from
	 */
	embed(nlohmann::json* j);

	/** Destructor */
	~embed();

	/** Set embed title. Returns the embed itself so these method calls may be "chained"
	 * @param text The text of the title
	 * @return A reference to self
	 */
	embed& set_title(const std::string &text);

	/** Set embed description. Returns the embed itself so these method calls may be "chained"
	 * @param text The text of the title
	 * @return A reference to self
	 */
	embed& set_description(const std::string &text);

	/** Set embed colour. Returns the embed itself so these method calls may be "chained"
	 * @param text The text of the title
	 * @return A reference to self
	 */
	embed& set_color(uint32_t col);

	/** Set embed url. Returns the embed itself so these method calls may be "chained"
	 * @param text The text of the title
	 * @return A reference to self
	 */
	embed& set_url(const std::string &url);

	/** Add an embed field. Returns the embed itself so these method calls may be "chained"
	 * @param text The text of the title
	 * @return A reference to self
	 */
	embed& add_field(const std::string& name, const std::string &value, bool is_inline);

	/** Set embed author. Returns the embed itself so these method calls may be "chained"
	 * @param text The text of the title
	 * @return A reference to self
	 */

	embed& set_author(const std::string& name, const std::string& url, const std::string& icon_url);

	/** Set embed provider. Returns the embed itself so these method calls may be "chained"
	 * @param text The text of the title
	 * @return A reference to self
	 */
	embed& set_provider(const std::string& name, const std::string& url);

	/** Set embed image. Returns the embed itself so these method calls may be "chained"
	 * @param text The text of the title
	 * @return A reference to self
	 */
	embed& set_image(const std::string& url);

	/** Set embed video. Returns the embed itself so these method calls may be "chained"
	 * @param text The text of the title
	 * @return A reference to self
	 */
	embed& set_video(const std::string& url);

	/** Set embed thumbnail. Returns the embed itself so these method calls may be "chained"
	 * @param text The text of the title
	 * @return A reference to self
	 */
	embed& set_thumbnail(const std::string& url);
};

/** Represets a reaction to a dpp::message */
struct reaction {
	/** Number of times this reaction has been added */
	uint32_t count;
	/** Reaction was from the bot's id */
	bool me;
	/** ID of emoji for reaction */
	snowflake emoji_id;
};

/** Represents messages sent and received on Discord */
struct message {
	/** id of the message */
	snowflake       id;
	/** id of the channel the message was sent in */
	snowflake       channel_id;
	/** Optional: id of the guild the message was sent in */
	snowflake       guild_id;
	/** the author of this message (not guaranteed to be a valid user) */
	user*		author;	
	/** Optional: member properties for this message's author */
	guild_member*	member;	
	/** contents of the message */
	std::string	content;
	/** when this message was sent */
	time_t		sent;
	/** when this message was edited (may be 0 if never edited) */
	time_t		edited;
	/** whether this was a TTS message */
	bool		tts;
	/** whether this message mentions everyone */
	bool   		mention_everyone;
	/** users specifically mentioned in the message */
	user*		mentions;
	/** roles specifically mentioned in this message */
	role*		mention_roles;
	/** Optional: channels specifically mentioned in this message */
	std::vector<snowflake> mention_channels;
	/** any attached files */
	std::vector<const unsigned char*> attachments;
	/** zero or more dpp::embed objects */
	std::vector<embed> embeds;
	/** Optional: reactions to the message */
	std::vector<reaction> reactions;
	/** Optional: used for validating a message was sent */
	std::string	nonce;
	/** whether this message is pinned */
	bool		pinned;	
	/** Optional: if the message is generated by a webhook, its id will be here otherwise the field will be 0 */
	snowflake	webhook_id;

	/** Name of file to upload (for use server-side in discord's url) */
	std::string	filename;
	/** File content to upload (raw binary) */
	std::string	filecontent;

	/** Fill this object from json.
	 * @param j JSON object to fill from
	 * @return A reference to self
	 */
	message& fill_from_json(nlohmann::json* j);

	/** Build JSON from this object.
	 * @param with_id True if the ID is to be included in the built JSON
	 * @return The JSON text of the message
	 */
	std::string build_json(bool with_id = false) const;
};

/** A group of messages */
typedef std::unordered_map<snowflake, message> message_map;

};
