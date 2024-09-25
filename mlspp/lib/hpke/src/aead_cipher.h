#pragma once

#include <hpke/hpke.h>

namespace mlspp::hpke {

struct ExportOnlyCipher : public AEAD
{
  ExportOnlyCipher();
  ~ExportOnlyCipher() override = default;

  bytes seal(const bytes& key,
             const bytes& nonce,
             const bytes& aad,
             const bytes& pt) const override;
  std::optional<bytes> open(const bytes& key,
                            const bytes& nonce,
                            const bytes& aad,
                            const bytes& ct) const override;
};

struct AEADCipher : public AEAD
{
  template<AEAD::ID id>
  static const AEADCipher& get();

  ~AEADCipher() override = default;

  bytes seal(const bytes& key,
             const bytes& nonce,
             const bytes& aad,
             const bytes& pt) const override;
  std::optional<bytes> open(const bytes& key,
                            const bytes& nonce,
                            const bytes& aad,
                            const bytes& ct) const override;

private:
  const size_t tag_size;

  AEADCipher(AEAD::ID id_in);
  friend AEADCipher make_aead(AEAD::ID cipher_in);
};

} // namespace mlspp::hpke
