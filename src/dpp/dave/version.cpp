#include "version.h"

namespace dpp::dave {

constexpr ProtocolVersion CurrentDaveProtocolVersion = 1;

ProtocolVersion MaxSupportedProtocolVersion()
{
    return CurrentDaveProtocolVersion;
}

} // namespace dpp::dave

