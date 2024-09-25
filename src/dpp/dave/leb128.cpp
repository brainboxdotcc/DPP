
#include "leb128.h"

// The following code was copied from the webrtc source code:
// https://webrtc.googlesource.com/src/+/refs/heads/main/modules/rtp_rtcp/source/leb128.cc

namespace discord {
namespace dave {

size_t Leb128Size(uint64_t value)
{
    int size = 0;
    while (value >= 0x80) {
        ++size;
        value >>= 7;
    }
    return size + 1;
}

uint64_t ReadLeb128(const uint8_t*& readAt, const uint8_t* end)
{
    uint64_t value = 0;
    int fillBits = 0;
    while (readAt != end && fillBits < 64 - 7) {
        uint8_t leb128Byte = *readAt;
        value |= uint64_t{leb128Byte & 0x7Fu} << fillBits;
        ++readAt;
        fillBits += 7;
        if ((leb128Byte & 0x80) == 0) {
            return value;
        }
    }
    // Read 9 bytes and didn't find the terminator byte. Check if 10th byte
    // is that terminator, however to fit result into uint64_t it may carry only
    // single bit.
    if (readAt != end && *readAt <= 1) {
        value |= uint64_t{*readAt} << fillBits;
        ++readAt;
        return value;
    }
    // Failed to find terminator leb128 byte.
    readAt = nullptr;
    return 0;
}

size_t WriteLeb128(uint64_t value, uint8_t* buffer)
{
    int size = 0;
    while (value >= 0x80) {
        buffer[size] = 0x80 | (value & 0x7F);
        ++size;
        value >>= 7;
    }
    buffer[size] = value;
    ++size;
    return size;
}

} // namespace dave
} // namespace discord
