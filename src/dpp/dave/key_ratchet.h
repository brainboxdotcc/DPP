#pragma once

#include <memory>
#include "common.h"

namespace discord {
namespace dave {

using KeyGeneration = uint32_t;

class IKeyRatchet {
public:
    virtual ~IKeyRatchet() noexcept = default;
    virtual EncryptionKey GetKey(KeyGeneration generation) noexcept = 0;
    virtual void DeleteKey(KeyGeneration generation) noexcept = 0;
};

} // namespace dave
} // namespace discord
