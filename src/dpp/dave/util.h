#pragma once

#include <string>
#include <bytes/bytes.h>

namespace dpp::dave::mls {

::mlspp::bytes_ns::bytes BigEndianBytesFrom(uint64_t value) noexcept;
uint64_t FromBigEndianBytes(const ::mlspp::bytes_ns::bytes& value) noexcept;

} // namespace dpp::dave::mls


