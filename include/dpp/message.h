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
#include <dpp/queues.h>
#include <dpp/snowflake.h>
#include <dpp/managed.h>
#include <dpp/user.h>
#include <dpp/guild.h>
#include <optional>
#include <variant>
#include <dpp/json_fwd.hpp>

namespace dpp {

/**
 * @brief Represents the type of a component
 */
enum component_type : uint8_t {
	/// Action row, a container for other components
	cot_action_row = 1,
	/// Clickable button
	cot_button = 2,
	/// Select menu
	cot_selectmenu = 3,
	/// Text input
	cot_text = 4,
};

/**
 * @brief Types of text input
 */
enum text_style_type : uint8_t {
	/// Intended for short single-line text.
	text_short = 1,
	/// Intended for much longer inputs.
	text_paragraph = 2,
};

/**
 * @brief Represents the style of a button
 */
enum component_style : uint8_t {
	/// Blurple
	cos_primary = 1,
	/// Grey
	cos_secondary,
	/// Green
	cos_success,
	/// Red
	cos_danger,
	/// An external hyperlink to a website
	cos_link
};

/**
 * @brief An option for a select component
 */
struct DPP_EXPORT select_option {
	/**
	 * @brief Label for option
	 */
	std::string label;
	/**
	 * @brief Value for option
	 */
	std::string value;
	/**
	 * @brief Description of option
	 */
	std::string description;
	/**
	 * @brief True if option is the default option
	 */
	bool is_default;
	/**
	 * @brief Emoji definition. To set an emoji on your button
	 * you must set one of either the name or id fields.
	 * The easiest way is to use the component::set_emoji
	 * method.
	 */
	struct inner_select_emoji {
		/**
		 * @brief Set the name field to the name of the emoji.
		 * For built in unicode emojis, set this to the
		 * actual unicode value of the emoji e.g. "😄"
		 * and not for example ":smile:"
		 */
		std::string name;
		/**
		 * @brief The emoji ID value for emojis that are custom
		 * ones belonging to a guild. The same rules apply
		 * as with other emojis, that the bot must be on
		 * the guild where the emoji resides and it must
		 * be available for use (e.g. not disabled due to
		 * lack of boosts etc)
		 */
		dpp::snowflake id = 0;
		/**
		 * @brief True if the emoji is animated. Only applies to
		 * custom emojis.
		 */
		bool animated = false;
	} emoji;

	/**
	 * @brief Construct a new select option object
	 */
	select_option();

	/**
	 * @brief Construct a new select option object
	 * 
	 * @param label Label of option
	 * @param value Value of option
	 * @param description Description of option
	 */
	select_option(const std::string &label, const std::string &value, const std::string &description = "");

	/**
	 * @brief Set the label
	 * 
	 * @param l the user-facing name of the option. It will be truncated to the maximum length of 100 UTF-8 characters.
	 * @return select_option& reference to self for chaining
	 */
	select_option& set_label(const std::string &l);

	/**
	 * @brief Set the value
	 * 
	 * @param v value to set. It will be truncated to the maximum length of 100 UTF-8 characters.
	 * @return select_option& reference to self for chaining
	 */
	select_option& set_value(const std::string &v);

	/**
	 * @brief Set the description
	 * 
	 * @param d description to set. It will be truncated to the maximum length of 100 UTF-8 characters.
	 * @return select_option& reference to self for chaining
	 */
	select_option& set_description(const std::string &d);

	/**
	 * @brief Set the emoji
	 * 
	 * @param n emoji name
	 * @param id emoji id for custom emojis
	 * @param animated true if animated emoji
	 * @return select_option& reference to self for chaining
	 */
	select_option& set_emoji(const std::string &n, dpp::snowflake id = 0, bool animated = false);

	/**
	 * @brief Set the option as default
	 * 
	 * @param def true to set the option as default
	 * @return select_option& reference to self for chaining
	 */
	select_option& set_default(bool def);

	/**
	 * @brief Set the emoji as animated
	 * 
	 * @param anim true if animated
	 * @return select_option& reference to self for chaining
	 */
	select_option& set_animated(bool anim);
};

/**
 * @brief Represents the component object.
 * A component is a clickable button or drop down list
 * within a discord message, where the buttons emit
 * on_button_click events when the user interacts with
 * them.
 *
 * You should generally define one component object and
 * then insert one or more additional components into it
 * using component::add_component(), so that the parent
 * object is an action row and the child objects are buttons.
 *
 * @note At present this only works for whitelisted
 * guilds. The beta is **closed**. When this feature is
 * released, then the functionality will work correctly.
 */
class DPP_EXPORT component {
public:
	/** Component type, either a button or action row
	 */
	component_type type;

	/** Sub components, buttons on an action row
	 */
	std::vector<component> components;

	/** Component label (for buttons, text inputs).
	 * Maximum of 80 characters.
	 */
	std::string label;

	/** Component style (for buttons)
	 */
	component_style style;

	/**
	 * @brief Text style (for text inputs)
	 */
	text_style_type text_style;

	/** Component id (for buttons, menus, text inputs).
	 * Maximum of 100 characters.
	 */
	std::string custom_id;

	/** URL for link types (dpp::cos_link).
	 * Maximum of 512 characters.
	 */
	std::string url;

	/** Placeholder text for select menus and text inputs (max 100 characters)
	 */
	std::string placeholder;

	/** Minimum number of selectable values for a select menu.
	 * -1 to not set this
	 */
	int32_t min_values;

	/** Maximum number of selectable values for a select menu.
	 * -1 to not set this.
	 */
	int32_t max_values;

	/** Minimum length for text input (0-4000)
	 */
	int32_t min_length;

	/** Maximum length for text input (1-4000)
	 */
	int32_t max_length;

	/** Select options for select menus
	 */
	std::vector<select_option> options;

	/** Disabled flag (for buttons)
	 */
	bool disabled;

	/** Whether the text input is required to be filled
	 */
	bool required;

	/** Current value (only filled or valid when populated from an on_form_submit event)
	 */
	std::variant<std::monostate, std::string, int64_t, double> value;

	/** Emoji definition. To set an emoji on your button
	 * you must set one of either the name or id fields.
	 * The easiest way is to use the component::set_emoji
	 * method.
	 */
	struct inner_emoji {
		/** Set the name field to the name of the emoji.
		 * For built in unicode emojis, set this to the
		 * actual unicode value of the emoji e.g. "😄"
		 * and not for example ":smile:"
		 */
		std::string name;
		/** The emoji ID value for emojis that are custom
		 * ones belonging to a guild. The same rules apply
		 * as with other emojis, that the bot must be on
		 * the guild where the emoji resides and it must
		 * be available for use (e.g. not disabled due to
		 * lack of boosts etc)
		 */
		dpp::snowflake id;
		/** True if the emoji is animated. Only applies to
		 * custom emojis.
		 */
		bool animated;
	} emoji;

	/** Constructor
	 */
	component();

	/** Destructor
	 */
	~component() = default;

	/**
	 * @brief Set the type of the component. Button components
	 * (type dpp::cot_button) should always be contained within
	 * an action row (type dpp::cot_action_row). As described
	 * below, many of the other methods automatically set this
	 * to the correct type so usually you should not need to
	 * manually  call component::set_type().
	 *
	 * @param ct The component type
	 * @return component& reference to self
	 */
	component& set_type(component_type ct);

	/**
	 * @brief Set the text style of a text component
	 * @note Sets type to `cot_text`
	 * 
	 * @param ts Text style type to set
	 * @return component& reference to self
	 */
	component& set_text_style(text_style_type ts);

	/**
	 * @brief Set the label of the component, e.g. button text.
	 * For action rows, this field is ignored. Setting the
	 * label will auto-set the type to dpp::cot_button.
	 *
	 * @param label Label text to set. It will be truncated to the maximum length of 80 UTF-8 characters.
	 * @return component& Reference to self
	 */
	component& set_label(const std::string &label);

	/**
	 * @brief Set the url for dpp::cos_link types.
	 * Calling this function sets the style to dpp::cos_link
	 * and the type to dpp::cot_button.
	 *
	 * @param url URL to set. It will be truncated to the maximum length of 512 UTF-8 characters.
	 * @return component& reference to self.
	 */
	component& set_url(const std::string &url);

	/**
	 * @brief Set the style of the component, e.g. button colour.
	 * For action rows, this field is ignored. Setting the
	 * style will auto-set the type to dpp::cot_button.
	 *
	 * @param cs Component style to set
	 * @return component& reference to self
	 */
	component& set_style(component_style cs);

	/**
	 * @brief Set the id of the component.
	 * For action rows, this field is ignored. Setting the
	 * id will auto-set the type to dpp::cot_button.
	 *
	 * @param id Custom ID string to set. This ID will be sent
	 * for any on_button_click events related to the button.
	 * @note The maximum length of the Custom ID is 100 UTF-8 codepoints.
	 * If your Custom ID is longer than this, it will be truncated.
	 * @return component& Reference to self
	 */
	component& set_id(const std::string &id);

	/**
	 * @brief Set the component to disabled.
	 * Defaults to false on all created components.
	 *
	 * @param disable True to disable, false to disable.
	 * @return component& Reference to self
	 */
	component& set_disabled(bool disable);

	/**
	 * @brief Set the placeholder
	 * 
	 * @param placeholder placeholder string. It will be truncated to the maximum length of 100 UTF-8 characters.
	 * @return component& Reference to self
	 */
	component& set_placeholder(const std::string &placeholder);

	/**
	 * @brief Set the min value
	 * 
	 * @param min_values min value to set
	 * @return component& Reference to self
	 */
	component& set_min_values(uint32_t min_values);

	/**
	 * @brief Set the max value
	 * 
	 * @param max_values max value to set
	 * @return component& Reference to self
	 */
	component& set_max_values(uint32_t max_values);

	/**
	 * @brief Set the min length of text input
	 * 
	 * @param min_l min value to set
	 * @return component& Reference to self
	 */
	component& set_min_length(uint32_t min_l);

	/**
	 * @brief Set the max length of text input
	 * 
	 * @param max_l max value to set
	 * @return component& Reference to self
	 */
	component& set_max_length(uint32_t max_l);

	/**
	 * @brief Add a select option
	 * 
	 * @param option option to add
	 * @return component& Reference to self
	 */
	component& add_select_option(const select_option &option);

	/**
	 * @brief Add a sub-component, only valid for action rows.
	 * Adding subcomponents to a component will automatically
	 * set this component's type to dpp::cot_action_row.
	 *
	 * @param c The sub-component to add
	 * @return component& reference to self
	 */
	component& add_component(const component& c);

	/**
	 * @brief Set the emoji of the current sub-component.
	 * Only valid for buttons. Adding an emoji to a component
	 * will automatically set this components type to
	 * dpp::cot_button. One or both of name and id must be set.
	 * For a built in unicode emoji, you only need set name,
	 * and should set it to a unicode character e.g. "😄".
	 * For custom emojis, set the name to the name of the emoji
	 * on the guild, and the id to the emoji's ID.
	 * Setting the animated boolean is only valid for custom
	 * emojis.
	 *
	 * @param name Emoji name, or unicode character to use
	 * @param id Emoji id, for custom emojis only.
	 * @param animated True if the custom emoji is animated.
	 * @return component& Reference to self
	 */
	component& set_emoji(const std::string& name, dpp::snowflake id = 0, bool animated = false);

	/** Read class values from json object
	 * @param j A json object to read from
	 * @return A reference to self
	 */
	component& fill_from_json(nlohmann::json* j);

};

/**
 * @brief A footer in a dpp::embed
 */
struct DPP_EXPORT embed_footer {
	/** Footer text */
	std::string text;
	/** Footer icon url */
	std::string icon_url;
	/** Proxied icon url */
	std::string proxy_url;

	/** Set footer's text. Returns footer itself so these methods may be "chained"
	 * @param t string to set as footer text. It will be truncated to the maximum length of 2048 UTF-8 characters.
	 * @return A reference to self
	 */
	embed_footer& set_text(const std::string& t);

	/** Set footer's icon url. Returns footer itself so these methods may be "chained"
	 * @param i url to set as footer icon url
	 * @return A reference to self
	 */
	embed_footer& set_icon(const std::string& i);

	/** Set footer's proxied icon url. Returns footer itself so these methods may be "chained"
	 * @param p url to set as footer proxied icon url
	 * @return A reference to self
	 */
	embed_footer& set_proxy(const std::string& p);
};

/**
 * @brief An video, image or thumbnail in a dpp::embed
 */
struct DPP_EXPORT embed_image {
	/** URL to image or video */
	std::string url;
	/** Proxied image url */
	std::string proxy_url;
	/** Height (calculated by discord) */
	std::string height;
	/** Width (calculated by discord) */
	std::string width;
};

/**
 * @brief Embed provider in a dpp::embed. Received from discord but cannot be sent
 */
struct DPP_EXPORT embed_provider {
	/** Provider name */
	std::string name;
	/** Provider URL */
	std::string url;
};

/**
 * @brief Author within a dpp::embed object
 */
struct DPP_EXPORT embed_author {
	/** Author name */
	std::string name;
	/** Author url */
	std::string url;
	/** Author icon url */
	std::string icon_url;
	/** Proxied icon url */
	std::string proxy_icon_url;
};

/**
 * @brief A dpp::embed may contain zero or more fields
 */
struct DPP_EXPORT embed_field {
	/** Name of field */
	std::string name;
	/** Value of field (max length 1000) */
	std::string value;
	/** True if the field is to be displayed inline */
	bool is_inline;
};

/**
 * @brief A rich embed for display within a dpp::message
 */
struct DPP_EXPORT embed {
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
	 * @param text The text of the title. It will be truncated to the maximum length of 256 UTF-8 characters.
	 * @return A reference to self
	 */
	embed& set_title(const std::string &text);

	/** Set embed description. Returns the embed itself so these method calls may be "chained"
	 * @param text The text of the title. It will be truncated to the maximum length of 4096 UTF-8 characters.
	 * @return A reference to self
	 */
	embed& set_description(const std::string &text);

	/** Set the footer of the embed. Returns the embed itself so these method calls may be "chained"
	 * @param f the footer to set
	 * @return A reference to self
	 */
	embed& set_footer(const embed_footer& f);

	 /** Set the footer of the embed. Returns the embed itself so these method calls may be "chained"
	  * @param text string to set as footer text. It will be truncated to the maximum length of 2048 UTF-8 characters.
	  * @param icon_url url to set as footer icon url
	  * @return A reference to self
	  */
	embed& set_footer(const std::string& text, const std::string& icon_url);

	/** Set embed colour. Returns the embed itself so these method calls may be "chained"
	 * @param col The colour of the embed
	 * @return A reference to self
	 */
	embed& set_color(uint32_t col);

	/** Set embed timestamp. Returns the embed itself so these method calls may be "chained"
	 * @param tstamp The timestamp to show in the footer, should be in UTC
	 * @return A reference to self
	 */
	embed& set_timestamp(time_t tstamp);

	/** Set embed url. Returns the embed itself so these method calls may be "chained"
	 * @param url the url of the embed
	 * @return A reference to self
	 */
	embed& set_url(const std::string &url);

	/** Add an embed field. Returns the embed itself so these method calls may be "chained"
	 * @param name The name of the field. It will be truncated to the maximum length of 256 UTF-8 characters.
	 * @param value The value of the field. It will be truncated to the maximum length of 1024 UTF-8 characters.
	 * @param is_inline Whether or not to display the field 'inline' or on its own line
	 * @return A reference to self
	 */
	embed& add_field(const std::string& name, const std::string &value, bool is_inline = false);

	/** Set embed author. Returns the embed itself so these method calls may be "chained" 
	 * @param a The author to set
	 * @return A reference to self
	 */ 
	embed& set_author(const dpp::embed_author& a);

	/** Set embed author. Returns the embed itself so these method calls may be "chained"
	 * @param name The name of the author. It will be truncated to the maximum length of 256 UTF-8 characters.
	 * @param url The url of the author
	 * @param icon_url The icon URL of the author
	 * @return A reference to self
	 */
	embed& set_author(const std::string& name, const std::string& url, const std::string& icon_url);

	/** Set embed provider. Returns the embed itself so these method calls may be "chained"
	 * @param name The provider name. It will be truncated to the maximum length of 256 UTF-8 characters.
	 * @param url The provider url
	 * @return A reference to self
	 */
	embed& set_provider(const std::string& name, const std::string& url);

	/** Set embed image. Returns the embed itself so these method calls may be "chained"
	 * @param url The embed image URL
	 * @return A reference to self
	 */
	embed& set_image(const std::string& url);

	/** Set embed video. Returns the embed itself so these method calls may be "chained"
	 * @param url The embed video url
	 * @return A reference to self
	 */
	embed& set_video(const std::string& url);

	/** Set embed thumbnail. Returns the embed itself so these method calls may be "chained"
	 * @param url The embed thumbnail url
	 * @return A reference to self
	 */
	embed& set_thumbnail(const std::string& url);
};

/**
 * @brief Represents a reaction to a dpp::message
 */
struct DPP_EXPORT reaction {
	/** Number of times this reaction has been added */
	uint32_t count;
	/** Reaction was from the bot's id */
	bool me;
	/** ID of emoji for reaction */
	snowflake emoji_id;
	/** Name of emoji, if applicable */
	std::string emoji_name;

	/**
	 * @brief Constructs a new reaction object.
	 */
	reaction();

	/**
	 * @brief Constructs a new reaction object from a JSON object.
	 * @param j The JSON to read data from
	 */
	reaction(nlohmann::json* j);

	/**
	 * @brief Destructs the reaction object.
	 */
	~reaction() = default;
};

/**
 * @brief Represents an attachment in a dpp::message
 */
struct DPP_EXPORT attachment {
	/** ID of attachment */
	snowflake id;
	/** Size of the attachment in bytes */
	uint32_t size;
	/** File name of the attachment */
	std::string filename;
	/** URL which points to the attachment */
	std::string url;
	/** Proxied URL which points to the attachment */
	std::string proxy_url;
	/** Width of the attachment, if applicable */
	uint32_t width;
	/** Height of the attachment, if applicable */
	uint32_t height;
	/** MIME type of the attachment, if applicable */
	std::string content_type;
	/** Whether this attachment is ephemeral, if applicable */
	bool ephemeral;
	/** Owning message */
	struct message* owner;

	/**
	 * @brief Constructs a new attachment object.
	 * @param o Owning dpp::message object
	 */
	attachment(struct message* o);

	/**
	 * @brief Constructs a new attachment object from a JSON object.
	 * @param o Owning dpp::message object
	 * @param j JSON to read information from
	 */
	attachment(struct message* o, nlohmann::json* j);

	/**
	 * @brief Destructs the attachment object.
	 */
	~attachment() = default;

	/**
	 * @brief Download this attachment
	 * @param callback A callback which is called when the download completes.
	 * @note The content of the file will be in the http_info.body parameter of the
	 * callback parameter.
	 * @throw dpp::exception If there is no owner associated with this attachment that
	 * itself has an owning cluster, this method will throw a dpp::exception when called.
	 */
	void download(http_completion_event callback) const;
};

/**
 * @brief Represents the type of a sticker
 */
enum sticker_type : uint8_t {
	/// Nitro pack sticker
	st_standard = 1,
	/// Guild sticker
	st_guild = 2
};

/**
 * @brief The file format (png, apng, lottie) of a sticker
 */
enum sticker_format : uint8_t {
	sf_png = 1,
	sf_apng = 2,
	sf_lottie = 3
};

/**
 * @brief Represents stickers received in messages
 */
struct DPP_EXPORT sticker : public managed {
	/** Optional: for standard stickers, id of the pack the sticker is from
	 */
	snowflake	pack_id;
	/** The name of the sticker */
	std::string	name;
	/// description of the sticker (may be empty)
	std::string	 description;	
	/** for guild stickers, the Discord name of a unicode emoji representing the sticker's expression.
	 * for standard stickers, a comma-separated list of related expressions.
	 */
	std::string	 tags;
	/**
	 * @brief Asset ID
	 * @deprecated now an empty string but still sent by discord.
	 * While discord still send this empty string value we will still have a field
	 * here in the library.
	 */
	std::string	 asset;
	/** The type of sticker */
	sticker_type	type;
	/// type of sticker format
	sticker_format	format_type;
	/// Optional: whether this guild sticker can be used, may be false due to loss of Server Boosts
	bool		available;
	/// Optional: id of the guild that owns this sticker
	snowflake	guild_id;
	/// Optional: the user that uploaded the guild sticker
	user		sticker_user;
	/// Optional: the standard sticker's sort order within its pack
	uint8_t		sort_value;
	/** Name of file to upload (when adding or editing a sticker) */
	std::string	filename;
	/** File content to upload (raw binary) */
	std::string	filecontent;

	/**
	 * @brief Construct a new sticker object
	 */
	sticker();

	/** Read class values from json object
	 * @param j A json object to read from
	 * @return A reference to self
	 */
	sticker& fill_from_json(nlohmann::json* j);

	/** Build JSON from this object.
	 * @param with_id True if the ID is to be set in the JSON structure
	 * @return The JSON text of the invite
	 */
	std::string build_json(bool with_id = true) const;

	/**
	 * @brief Set the filename
	 * 
	 * @param fn filename
	 * @return message& reference to self
	 */
	sticker& set_filename(const std::string &fn);

	/**
	 * @brief Set the file content
	 * 
	 * @param fc raw file content contained in std::string
	 * @return message& reference to self
	 */
	sticker& set_file_content(const std::string &fc);

};

/**
 * @brief Represents a sticker pack (the built in groups of stickers that all nitro users get to use)
 */
struct DPP_EXPORT sticker_pack : public managed {
	/// the stickers in the pack
	std::map<snowflake, sticker> stickers;
	/// name of the sticker pack
	std::string	 name;
	/// id of the pack's SKU
	snowflake	   sku_id;
	/// Optional: id of a sticker in the pack which is shown as the pack's icon
	snowflake	   cover_sticker_id;
	/// description of the sticker pack
	std::string	 description;
	/// id of the sticker pack's banner image
	snowflake	   banner_asset_id;

	/**
	 * @brief Construct a new sticker pack object
	 */
	sticker_pack();

	/** Read class values from json object
	 * @param j A json object to read from
	 * @return A reference to self
	 */
	sticker_pack& fill_from_json(nlohmann::json* j);

	/** Build JSON from this object.
	 * @param with_id True if the ID is to be set in the JSON structure
	 * @return The JSON text of the invite
	 */
	std::string build_json(bool with_id = true) const;

};

/**
 * @brief Bitmask flags for a dpp::message
 */
enum message_flags {
	/// this message has been published to subscribed channels (via Channel Following)
	m_crossposted = 1 << 0,
	/// this message originated from a message in another channel (via Channel Following)
	m_is_crosspost =  1 << 1,
	/// do not include any embeds when serializing this message
	m_suppress_embeds = 1 << 2,
	/// the source message for this crosspost has been deleted (via Channel Following)
	m_source_message_deleted = 1 << 3,
	/// this message came from the urgent message system
	m_urgent = 1 << 4,
	/// this message has an associated thread, with the same id as the message
	m_has_thread = 1 << 5,
	/// this message is only visible to the user who invoked the Interaction
	m_ephemeral = 1 << 6,
	/// this message is an Interaction Response and the bot is "thinking"
	m_loading = 1 << 7
};

/**
 * @brief Message types for dpp::message::type
 */
enum message_type {
	/// Default
	mt_default					= 0,
	/// Add recipient
	mt_recipient_add				= 1,
	/// Remove recipient
	mt_recipient_remove				= 2,
	/// Call
	mt_call						= 3,
	/// Channel name change
	mt_channel_name_change				= 4,
	/// Channel icon change
	mt_channel_icon_change				= 5,
	/// Message pinned
	mt_channel_pinned_message			= 6,
	/// Member joined
	mt_guild_member_join				= 7,
	/// Boost
	mt_user_premium_guild_subscription		= 8,
	/// Boost level 1
	mt_user_premium_guild_subscription_tier_1	= 9,
	/// Boost level 2
	mt_user_premium_guild_subscription_tier_2	= 10,
	/// Boost level 3
	mt_user_premium_guild_subscription_tier_3	= 11,
	/// Follow channel
	mt_channel_follow_add				= 12,
	/// Disqualified from discovery
	mt_guild_discovery_disqualified			= 14,
	/// Re-qualified for discovery
	mt_guild_discovery_requalified			= 15,
	/// Discovery grace period warning 1
	mt_guild_discovery_grace_period_initial_warning	= 16,
	/// Discovery grace period warning 2
	mt_guild_discovery_grace_period_final_warning	= 17,
	/// Thread Created 
	mt_thread_created			= 18,
	/// Reply
	mt_reply					= 19,
	/// Application command
	mt_application_command				= 20,
	/// Thread starter message
	mt_thread_starter_message	= 21,
	/// Invite reminder
	mt_guild_invite_reminder			= 22,
	/// Context Menu Command
	mt_context_menu_command 	= 23
};

/**
 * @brief Represents the caching policy of a cache in the library.
 */
enum cache_policy_setting_t {
	/**
	 * @brief request aggressively on seeing new guilds, and also store missing data from messages.
	 * This is the default behaviour and the least memory-efficient option. Memory usage will increase
	 * over time, initially quite rapidly, and then linearly over time. It is the least cpu-intensive
	 * setting.
	 */
	cp_aggressive = 0,
	/**
	 * @brief only cache when there is relevant activity, e.g. a message to the bot.
	 * This is a good middle-ground, memory usage will increase linearly over time.
	 */
	cp_lazy = 1,
	/**
	 * @brief Don't cache anything. Fill details when we see them.
	 * This is the most memory-efficient option but consumes more CPU time
	 */
	cp_none = 2
};

/**
 * @brief Represents the caching policy of the cluster.
 * 
 * Channels and guilds are always cached as these caches are used
 * internally by the library. The memory usage of these is minimal.
 * 
 * All default to 'aggressive' which means to actively attempt to cache,
 * going out of the way to fill the caches completely. On large bots this
 * can take a LOT of RAM.
 */
struct DPP_EXPORT cache_policy_t {
	/**
	 * @brief Caching policy for users and guild members
	 */
	cache_policy_setting_t user_policy = cp_aggressive;

	/**
	 * @brief Caching policy for emojis
	 */
	cache_policy_setting_t emoji_policy = cp_aggressive;

	/**
	 * @brief Caching policy for roles
	 */
	cache_policy_setting_t role_policy = cp_aggressive;
};

/**
 * @brief Represents messages sent and received on Discord
 */
struct DPP_EXPORT message : public managed {
	/** id of the channel the message was sent in */
	snowflake	   channel_id;
	/** Optional: id of the guild the message was sent in */
	snowflake	   guild_id;
	/** the author of this message (not guaranteed to be a valid user) */
	user		author;
	/** Optional: member properties for this message's author */
	guild_member	member;
	/** contents of the message */
	std::string	content;
	/** message components */
	std::vector<dpp::component> components;
	/** when this message was sent */
	time_t		sent;
	/** when this message was edited (may be 0 if never edited) */
	time_t		edited;
	/** whether this was a TTS message */
	bool		tts;
	/** whether this message mentions everyone */
	bool   		mention_everyone;
	/** users specifically mentioned in the message */
	std::vector<std::pair<user, guild_member>>	mentions;
	/** roles specifically mentioned in this message (only IDs currently)*/
	std::vector<snowflake> mention_roles;
	/** Channels mentioned in the message. (Discord: not all types supported)
	 * Discord: Only textual channels that are visible to everyone in a lurkable guild will ever be included. Only crossposted messages (via Channel Following) currently include mention_channels at all. (includes ID, Guild ID, Type, Name)*/
	std::vector<channel> mention_channels;
	/** any attached files */
	std::vector<attachment> attachments;
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
	/** Flags */
	uint8_t		flags;
	/** Stickers */
	std::vector<sticker> stickers;

	/** Name of file to upload (for use server-side in discord's url) */
	std::vector<std::string>	filename;

	/** File content to upload (raw binary) */
	std::vector<std::string>	filecontent;

	/** Message type */
	message_type type;

	/**
	 * @brief Reference to another message, e.g. a reply
	 */
	struct message_ref {
		/// id of the originating message
		snowflake message_id;
		/// id of the originating message's channel
		snowflake channel_id;
		/// id of the originating message's guild
		snowflake guild_id;
		/// when sending, whether to error if the referenced message doesn't exist instead of sending as a normal (non-reply) message, default true
		bool fail_if_not_exists;
	} message_reference;

	/**
	 * @brief Reference to an interaction
	 */
	struct message_interaction_struct{
		/// id of the interaction
		snowflake id;
		/// type of interaction
		uint8_t type;
		/// name of the application command
		std::string name;
		/// the user who invoked the interaction
		user usr;
	} interaction;

	/**
	 * @brief Allowed mentions details
	 */
	struct allowed_ref {
		/**
		 * @brief Set to true to parse user mentions in the text
		 */
		bool parse_users;
		/**
		 * @brief Set to true to at-everyone and at-here mentions in the text
		 */
		bool parse_everyone;
		/**
		 * @brief Set to true to parse role mentions in the text
		 */
		bool parse_roles;
		/**
		 * @brief Set to true to mention the user who sent the message this one is replying to
		 */
		bool replied_user;
		/**
		 * @brief List of users to allow pings for 
		 */
		std::vector<snowflake> users;
		/**
		 * @brief List of roles to allow pings for 
		 */
		std::vector<snowflake> roles;
	} allowed_mentions;

	/**
	 * @brief The cluster which created this message object
	 */
	class cluster* owner;

	/**
	 * @brief Construct a new message object
	 */
	message();

	/**
	 * @brief Construct a new message object
	 * @param o Owning cluster, passed down to various things such as dpp::attachment.
	 * Owning cluster is optional (can be nullptr) and if nulled, will prevent some functions
	 * such as attachment::download from functioning (they will throw, if used)
	 */
	message(class cluster* o);

	/**
	 * @brief Destroy the message object
	 */
	~message();

	/**
	 * @brief Construct a new message object with a channel and content
	 *
	 * @param channel_id The channel to send the message to
	 * @param content The content of the message. It will be truncated to the maximum length of 2000 UTF-8 characters.
	 * @param type The message type to create
	 */
	message(snowflake channel_id, const std::string &content, message_type type = mt_default);

	/**
	 * @brief Construct a new message object with a channel and content
	 *
	 * @param channel_id The channel to send the message to
	 * @param _embed An embed to send
	 */
	message(snowflake channel_id, const embed & _embed);

	/**
	 * @brief Construct a new message object with content
	 *
	 * @param content The content of the message. It will be truncated to the maximum length of 2000 UTF-8 characters.
	 * @param type The message type to create
	 */
	message(const std::string &content, message_type type = mt_default);

	/**
	 * @brief Set the original message reference for replies/crossposts
	 * 
	 * @param _message_id message id to reply to
	 * @param _guild_id guild id to reply to (optional)
	 * @param _channel_id channel id to reply to (optional)
	 * @param fail_if_not_exists true if the message send should fail if these values are invalid (optional)
	 * @return message& reference to self
	 */
	message& set_reference(snowflake _message_id, snowflake _guild_id = 0, snowflake _channel_id = 0, bool fail_if_not_exists = false);

	/**
	 * @brief Set the allowed mentions object for pings on the message
	 * 
	 * @param _parse_users whether or not to parse users in the message content or embeds
	 * @param _parse_roles whether or not to parse roles in the message content or embeds
	 * @param _parse_everyone whether or not to parse everyone/here in the message content or embeds 
	 * @param _replied_user if set to true and this is a reply, then ping the user we reply to
	 * @param users list of user ids to allow pings for
	 * @param roles list of role ids to allow pings for
	 * @return message& reference to self
	 */
	message& set_allowed_mentions(bool _parse_users, bool _parse_roles, bool _parse_everyone, bool _replied_user, const std::vector<snowflake> &users, const std::vector<snowflake> &roles);

	/** Fill this object from json.
	 * @param j JSON object to fill from
	 * @param cp Cache policy for user records, whether or not we cache users when a message is received
	 * @return A reference to self
	 */
	message& fill_from_json(nlohmann::json* j, cache_policy_t cp = {cp_aggressive, cp_aggressive, cp_aggressive});

	/** Build JSON from this object.
	 * @param with_id True if the ID is to be included in the built JSON
	 * @param is_interaction_response Set to true if this message is intended to be included in an interaction response.
	 * This will exclude some fields that are not valid in interactions at this time.
	 * @return The JSON text of the message
	 */
	std::string build_json(bool with_id = false, bool is_interaction_response = false) const;

	/**
	 * @brief Returns true if the message was crossposted to other servers
	 * 
	 * @return true if crossposted
	 */
	bool is_crossposted() const;

	/**
	 * @brief Returns true if posted from other servers news channel via webhook
	 * 
	 * @return true if posted from other server
	 */
	bool is_crosspost() const;

	/**
	 * @brief True if embeds have been removed
	 * 
	 * @return true if embeds removed
	 */
	bool suppress_embeds() const;

	/**
	 * @brief True if source message was deleted
	 * 
	 * @return true if source message deleted
	 */
	bool is_source_message_deleted() const;

	/**
	 * @brief True if urgent
	 * 
	 * @return true if urgent
	 */
	bool is_urgent() const;

	/**
	 * @brief True if has thread attached
	 *
	 * @return true if has thread attached
	 */
	bool has_thread() const;

	/**
	 * @brief True if ephemeral (visible only to issuer of a slash command)
	 * 
	 * @return true if ephemeral
	 */
	bool is_ephemeral() const;

	/**
	 * @brief True if loading
	 * 
	 * @return true if loading
	 */
	bool is_loading() const;

	/**
	 * @brief Add a component (button) to message
	 * 
	 * @param c component to add
	 * @return message& reference to self
	 */
	message& add_component(const component& c);

	/**
	 * @brief Add an embed to message
	 * 
	 * @param e embed to add
	 * @return message& reference to self
	 */
	message& add_embed(const embed& e);

	/**
	 * @brief Set the flags
	 * 
	 * @param f flags to set
	 * @return message& reference to self
	 */
	message& set_flags(uint8_t f);

	/**
	 * @brief Set the message type
	 * 
	 * @param t type to set
	 * @return message& reference to self
	 */
	message& set_type(message_type t);

	/**
	 * @brief Set the filename of the last file in list
	 * 
	 * @param fn filename
	 * @return message& reference to self
	 */
	message& set_filename(const std::string &fn);

	/**
	 * @brief Set the file content of the last file in list
	 * 
	 * @param fc raw file content contained in std::string
	 * @return message& reference to self
	 */
	message& set_file_content(const std::string &fc);

	/**
	 * @brief Add a file to the message
	 *
	 * @param filename filename
	 * @param filecontent raw file content contained in std::string
	 * @return message& reference to self
	 */
	message& add_file(const std::string &filename, const std::string &filecontent);

	/**
	 * @brief Set the message content
	 * 
	 * @param c message content. It will be truncated to the maximum length of 2000 UTF-8 characters.
	 * @return message& reference to self
	 */
	message& set_content(const std::string &c);
};

/** A group of messages */
typedef std::unordered_map<snowflake, message> message_map;

/** A group of stickers */
typedef std::unordered_map<snowflake, sticker> sticker_map;

/** A group of sticker packs */
typedef std::unordered_map<snowflake, sticker_pack> sticker_pack_map;

};
