#include "common.h"

namespace mlspp::hpke {

bytes
i2osp(uint64_t val, size_t size)
{
  auto out = bytes(size, 0);
  auto max = size;
  if (size > 8) {
    max = 8;
  }

  for (size_t i = 0; i < max; i++) {
    out.at(size - i - 1) = static_cast<uint8_t>(val >> (8 * i));
  }
  return out;
}

} // namespace mlspp::hpke
