#pragma once

#include <cstddef>
#include <cstdint>

namespace discord {
namespace dave {

constexpr size_t Leb128MaxSize = 10;

// Returns number of bytes needed to store `value` in leb128 format.
size_t Leb128Size(uint64_t value);

// Reads leb128 encoded value and advance read_at by number of bytes consumed.
// Sets read_at to nullptr on error.
uint64_t ReadLeb128(const uint8_t*& readAt, const uint8_t* end);

// Encodes `value` in leb128 format. Assumes buffer has size of at least
// Leb128Size(value). Returns number of bytes consumed.
size_t WriteLeb128(uint64_t value, uint8_t* buffer);

} // namespace dave
} // namespace discord
