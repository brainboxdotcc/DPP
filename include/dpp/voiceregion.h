#pragma once
#include <dpp/discord.h>
#include <dpp/json_fwd.hpp>

namespace dpp {

enum voiceregion_flags {
	v_optimal	= 0x00000001,
	v_deprecated	= 0x00000010,
	v_custom	= 0x00000100,
	v_vip		= 0x00001000
};

/**
 * @brief Represents a voice region on discord
 */
class voiceregion {
public:
	std::string id;
	std::string name;
	uint8_t flags;

	/**
	 * @brief Construct a new voiceregion object
	 */
	voiceregion();

	/**
	 * @brief Destroy the voiceregion object
	 */
	~voiceregion();

	/**
	 * @brief Fill object properties from JSON
	 * 
	 * @param j JSON to fill from
	 * @return voiceregion& Reference to self
	 */
	voiceregion& fill_from_json(nlohmann::json* j);

	/**
	 * @brief Build a json string for this object
	 * 
	 * @return std::string JSON string
	 */
	std::string build_json() const;

	bool is_optimal() const;
	bool is_deprecated() const;
	bool is_custom() const;
	bool is_vip() const;
};

/**
 * @brief A group of voice regions
 */
typedef std::unordered_map<std::string, voiceregion> voiceregion_map;

};
