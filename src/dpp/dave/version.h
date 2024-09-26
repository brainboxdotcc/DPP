#pragma once

#include <cstdint>

namespace dpp::dave {

using ProtocolVersion = uint16_t;
using SignatureVersion = uint8_t;

ProtocolVersion MaxSupportedProtocolVersion();

} // namespace dpp::dave

