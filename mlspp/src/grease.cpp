#include "grease.h"

#include <random>
#include <set>

namespace mlspp {

#ifdef DISABLE_GREASE

void
grease([[maybe_unused]] Capabilities& capabilities,
       [[maybe_unused]] const ExtensionList& extensions)
{
}

void
grease([[maybe_unused]] ExtensionList& extensions)
{
}

#else

// Randomness parmeters:
// * Given a list of N items, insert max(1, rand(p_grease * N)) GREASE values
// * Each GREASE value added is distinct, unless more than 15 values are needed
// * For extensions, each GREASE extension has rand(n_grease_ext) random bytes
//   of data
const size_t log_p_grease = 1; // -log2(p_grease) => p_grease = 1/2
const size_t max_grease_ext_size = 16;

const std::array<uint16_t, 15> grease_values = { 0x0A0A, 0x1A1A, 0x2A2A, 0x3A3A,
                                                 0x4A4A, 0x5A5A, 0x6A6A, 0x7A7A,
                                                 0x8A8A, 0x9A9A, 0xAAAA, 0xBABA,
                                                 0xCACA, 0xDADA, 0xEAEA };

static size_t
rand_int(size_t n)
{
  static auto seed = std::random_device()();
  static auto rng = std::mt19937(seed);
  return std::uniform_int_distribution<size_t>(0, n)(rng);
}

static uint16_t
grease_value()
{
  const auto where = rand_int(grease_values.size() - 1);
  return grease_values.at(where);
}

static bool
grease_value(uint16_t val)
{
  static constexpr auto grease_mask = uint16_t(0x0F0F);
  return ((val & grease_mask) == 0x0A0A) && val != 0xFAFA;
}

static std::set<uint16_t>
grease_sample(size_t count)
{
  auto vals = std::set<uint16_t>{};

  while (vals.size() < count) {
    uint16_t val = grease_value();
    while (vals.count(val) > 0 && vals.size() < grease_values.size()) {
      val = grease_value();
    }

    vals.insert(val);
  }

  return vals;
}

template<typename T>
static void
grease(std::vector<T>& vec)
{
  const auto count = std::max(size_t(1), rand_int(vec.size() >> log_p_grease));
  for (const auto val : grease_sample(count)) {
    const auto where = static_cast<ptrdiff_t>(rand_int(vec.size()));
    vec.insert(std::begin(vec) + where, static_cast<T>(val));
  }
}

void
grease(Capabilities& capabilities, const ExtensionList& extensions)
{
  // Add GREASE to the appropriate portions of the capabilities
  grease(capabilities.cipher_suites);
  grease(capabilities.extensions);
  grease(capabilities.proposals);
  grease(capabilities.credentials);

  // Ensure that the GREASE extensions are reflected in Capabilities.extensions
  for (const auto& ext : extensions.extensions) {
    if (!grease_value(ext.type)) {
      continue;
    }

    if (stdx::contains(capabilities.extensions, ext.type)) {
      continue;
    }

    const auto where =
      static_cast<ptrdiff_t>(rand_int(capabilities.extensions.size()));
    const auto where_ptr = std::begin(capabilities.extensions) + where;
    capabilities.extensions.insert(where_ptr, ext.type);
  }
}

void
grease(ExtensionList& extensions)
{
  auto& ext = extensions.extensions;
  const auto count = std::max(size_t(1), rand_int(ext.size() >> log_p_grease));
  for (const auto ext_type : grease_sample(count)) {
    const auto where = static_cast<ptrdiff_t>(rand_int(ext.size()));
    auto ext_data = random_bytes(rand_int(max_grease_ext_size));
    ext.insert(std::begin(ext) + where, { ext_type, std::move(ext_data) });
  }
}

#endif // DISABLE_GREASE

} // namespace mlspp
