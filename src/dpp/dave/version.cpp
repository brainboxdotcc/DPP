#include "version.h"

namespace discord {
namespace dave {

constexpr ProtocolVersion CurrentDaveProtocolVersion = 1;

ProtocolVersion MaxSupportedProtocolVersion()
{
    return CurrentDaveProtocolVersion;
}

} // namespace dave
} // namespace discord
