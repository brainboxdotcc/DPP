#include "util.h"

namespace discord {
namespace dave {
namespace mls {

::mlspp::bytes_ns::bytes BigEndianBytesFrom(uint64_t value) noexcept
{
    auto buffer = ::mlspp::bytes_ns::bytes();
    buffer.reserve(sizeof(value));

    for (int i = sizeof(value) - 1; i >= 0; --i) {
        buffer.push_back(static_cast<uint8_t>(value >> (i * 8)));
    }

    return buffer;
}

uint64_t FromBigEndianBytes(const ::mlspp::bytes_ns::bytes& buffer) noexcept
{
    uint64_t val = 0;

    if (buffer.size() <= sizeof(val)) {
        for (uint8_t byte : buffer) {
            val = (val << 8) | byte;
        }
    }

    return val;
}

} // namespace mls
} // namespace dave
} // namespace discord
