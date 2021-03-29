#pragma once

#include <dpp/json_fwd.hpp>

uint64_t SnowflakeNotNull(nlohmann::json* j, const char *keyname);
std::string StringNotNull(nlohmann::json* j, const char *keyname);
uint32_t Int32NotNull(nlohmann::json* j, const char *keyname);
uint16_t Int16NotNull(nlohmann::json* j, const char *keyname);
uint8_t Int8NotNull(nlohmann::json* j, const char *keyname);
bool BoolNotNull(nlohmann::json* j, const char *keyname);

