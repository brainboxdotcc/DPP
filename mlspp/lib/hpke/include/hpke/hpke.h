#pragma once

#include <memory>
#include <optional>

#include <bytes/bytes.h>
using namespace mlspp::bytes_ns;

namespace mlspp::hpke {

struct KEM
{
  enum struct ID : uint16_t
  {
    DHKEM_P256_SHA256 = 0x0010,
    DHKEM_P384_SHA384 = 0x0011,
    DHKEM_P521_SHA512 = 0x0012,
    DHKEM_X25519_SHA256 = 0x0020,
#if !defined(WITH_BORINGSSL)
    DHKEM_X448_SHA512 = 0x0021,
#endif
  };

  template<KEM::ID>
  static const KEM& get();

  virtual ~KEM() = default;

  struct PublicKey
  {
    virtual ~PublicKey() = default;
  };

  struct PrivateKey
  {
    virtual ~PrivateKey() = default;
    virtual std::unique_ptr<PublicKey> public_key() const = 0;
  };

  const ID id;
  const size_t secret_size;
  const size_t enc_size;
  const size_t pk_size;
  const size_t sk_size;

  virtual std::unique_ptr<PrivateKey> generate_key_pair() const = 0;
  virtual std::unique_ptr<PrivateKey> derive_key_pair(
    const bytes& ikm) const = 0;

  virtual bytes serialize(const PublicKey& pk) const = 0;
  virtual std::unique_ptr<PublicKey> deserialize(const bytes& enc) const = 0;

  virtual bytes serialize_private(const PrivateKey& sk) const;
  virtual std::unique_ptr<PrivateKey> deserialize_private(
    const bytes& skm) const;

  // (shared_secret, enc)
  virtual std::pair<bytes, bytes> encap(const PublicKey& pkR) const = 0;
  virtual bytes decap(const bytes& enc, const PrivateKey& skR) const = 0;

  // (shared_secret, enc)
  virtual std::pair<bytes, bytes> auth_encap(const PublicKey& pkR,
                                             const PrivateKey& skS) const;
  virtual bytes auth_decap(const bytes& enc,
                           const PublicKey& pkS,
                           const PrivateKey& skR) const;

protected:
  KEM(ID id_in,
      size_t secret_size_in,
      size_t enc_size_in,
      size_t pk_size_in,
      size_t sk_size_in);
};

struct KDF
{
  enum struct ID : uint16_t
  {
    HKDF_SHA256 = 0x0001,
    HKDF_SHA384 = 0x0002,
    HKDF_SHA512 = 0x0003,
  };

  template<KDF::ID id>
  static const KDF& get();

  virtual ~KDF() = default;

  const ID id;
  const size_t hash_size;

  virtual bytes extract(const bytes& salt, const bytes& ikm) const = 0;
  virtual bytes expand(const bytes& prk,
                       const bytes& info,
                       size_t size) const = 0;

  bytes labeled_extract(const bytes& suite_id,
                        const bytes& salt,
                        const bytes& label,
                        const bytes& ikm) const;
  bytes labeled_expand(const bytes& suite_id,
                       const bytes& prk,
                       const bytes& label,
                       const bytes& info,
                       size_t size) const;

protected:
  KDF(ID id_in, size_t hash_size_in);
};

struct AEAD
{
  enum struct ID : uint16_t
  {
    AES_128_GCM = 0x0001,
    AES_256_GCM = 0x0002,
    CHACHA20_POLY1305 = 0x0003,

    // Reserved identifier for pseudo-AEAD on contexts that only allow export
    export_only = 0xffff,
  };

  template<AEAD::ID id>
  static const AEAD& get();

  virtual ~AEAD() = default;

  const ID id;
  const size_t key_size;
  const size_t nonce_size;

  virtual bytes seal(const bytes& key,
                     const bytes& nonce,
                     const bytes& aad,
                     const bytes& pt) const = 0;
  virtual std::optional<bytes> open(const bytes& key,
                                    const bytes& nonce,
                                    const bytes& aad,
                                    const bytes& ct) const = 0;

protected:
  AEAD(ID id_in, size_t key_size_in, size_t nonce_size_in);
};

struct Context
{
  bytes do_export(const bytes& exporter_context, size_t size) const;

protected:
  bytes suite;
  bytes key;
  bytes nonce;
  bytes exporter_secret;
  const KDF& kdf;
  const AEAD& aead;

  bytes current_nonce() const;
  void increment_seq();

private:
  uint64_t seq;

  Context(bytes suite_in,
          bytes key_in,
          bytes nonce_in,
          bytes exporter_secret_in,
          const KDF& kdf_in,
          const AEAD& aead_in);

  friend struct HPKE;
  friend struct HPKETest;
  friend bool operator==(const Context& lhs, const Context& rhs);
};

struct SenderContext : public Context
{
  SenderContext(Context&& c);
  bytes seal(const bytes& aad, const bytes& pt);
};

struct ReceiverContext : public Context
{
  ReceiverContext(Context&& c);
  std::optional<bytes> open(const bytes& aad, const bytes& ct);
};

struct HPKE
{
  enum struct Mode : uint8_t
  {
    base = 0,
    psk = 1,
    auth = 2,
    auth_psk = 3,
  };

  HPKE(KEM::ID kem_id, KDF::ID kdf_id, AEAD::ID aead_id);

  using SenderInfo = std::pair<bytes, SenderContext>;

  SenderInfo setup_base_s(const KEM::PublicKey& pkR, const bytes& info) const;
  ReceiverContext setup_base_r(const bytes& enc,
                               const KEM::PrivateKey& skR,
                               const bytes& info) const;

  SenderInfo setup_psk_s(const KEM::PublicKey& pkR,
                         const bytes& info,
                         const bytes& psk,
                         const bytes& psk_id) const;
  ReceiverContext setup_psk_r(const bytes& enc,
                              const KEM::PrivateKey& skR,
                              const bytes& info,
                              const bytes& psk,
                              const bytes& psk_id) const;

  SenderInfo setup_auth_s(const KEM::PublicKey& pkR,
                          const bytes& info,
                          const KEM::PrivateKey& skS) const;
  ReceiverContext setup_auth_r(const bytes& enc,
                               const KEM::PrivateKey& skR,
                               const bytes& info,
                               const KEM::PublicKey& pkS) const;

  SenderInfo setup_auth_psk_s(const KEM::PublicKey& pkR,
                              const bytes& info,
                              const bytes& psk,
                              const bytes& psk_id,
                              const KEM::PrivateKey& skS) const;
  ReceiverContext setup_auth_psk_r(const bytes& enc,
                                   const KEM::PrivateKey& skR,
                                   const bytes& info,
                                   const bytes& psk,
                                   const bytes& psk_id,
                                   const KEM::PublicKey& pkS) const;

  bytes suite;
  const KEM& kem;
  const KDF& kdf;
  const AEAD& aead;

private:
  static bool verify_psk_inputs(Mode mode,
                                const bytes& psk,
                                const bytes& psk_id);
  Context key_schedule(Mode mode,
                       const bytes& shared_secret,
                       const bytes& info,
                       const bytes& psk,
                       const bytes& psk_id) const;
};

} // namespace mlspp::hpke
