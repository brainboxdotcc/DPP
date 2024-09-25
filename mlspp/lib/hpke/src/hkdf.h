#pragma once

#include <hpke/digest.h>
#include <hpke/hpke.h>

namespace mlspp::hpke {

struct HKDF : public KDF
{
  template<Digest::ID digest_id>
  static const HKDF& get();

  ~HKDF() override = default;

  bytes extract(const bytes& salt, const bytes& ikm) const override;
  bytes expand(const bytes& prk, const bytes& info, size_t size) const override;

private:
  const Digest& digest;

  explicit HKDF(const Digest& digest_in);
};

} // namespace mlspp::hpke
