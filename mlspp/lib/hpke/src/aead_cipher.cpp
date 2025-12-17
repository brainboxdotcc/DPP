#include "aead_cipher.h"
#include "openssl_common.h"

#include <openssl/evp.h>

#if WITH_BORINGSSL
#include <openssl/aead.h>
#endif

namespace mlspp::hpke {

///
/// ExportOnlyCipher
///
bytes
ExportOnlyCipher::seal(const bytes& /* key */,
                       const bytes& /* nonce */,
                       const bytes& /* aad */,
                       const bytes& /* pt */) const
{
  throw std::runtime_error("seal() on export-only context");
}

std::optional<bytes>
ExportOnlyCipher::open(const bytes& /* key */,
                       const bytes& /* nonce */,
                       const bytes& /* aad */,
                       const bytes& /* ct */) const
{
  throw std::runtime_error("open() on export-only context");
}

ExportOnlyCipher::ExportOnlyCipher()
  : AEAD(AEAD::ID::export_only, 0, 0)
{
}

///
/// AEADCipher
///
AEADCipher
make_aead(AEAD::ID cipher_in)
{
  return { cipher_in };
}

template<>
const AEADCipher&
AEADCipher::get<AEAD::ID::AES_128_GCM>()
{
  static const auto instance = make_aead(AEAD::ID::AES_128_GCM);
  return instance;
}

template<>
const AEADCipher&
AEADCipher::get<AEAD::ID::AES_256_GCM>()
{
  static const auto instance = make_aead(AEAD::ID::AES_256_GCM);
  return instance;
}

template<>
const AEADCipher&
AEADCipher::get<AEAD::ID::CHACHA20_POLY1305>()
{
  static const auto instance = make_aead(AEAD::ID::CHACHA20_POLY1305);
  return instance;
}

static size_t
cipher_key_size(AEAD::ID cipher)
{
  switch (cipher) {
    case AEAD::ID::AES_128_GCM:
      return 16;

    case AEAD::ID::AES_256_GCM:
    case AEAD::ID::CHACHA20_POLY1305:
      return 32;

    default:
      throw std::runtime_error("Unsupported algorithm");
  }
}

static size_t
cipher_nonce_size(AEAD::ID cipher)
{
  switch (cipher) {
    case AEAD::ID::AES_128_GCM:
    case AEAD::ID::AES_256_GCM:
    case AEAD::ID::CHACHA20_POLY1305:
      return 12;

    default:
      throw std::runtime_error("Unsupported algorithm");
  }
}

static size_t
cipher_tag_size(AEAD::ID cipher)
{
  switch (cipher) {
    case AEAD::ID::AES_128_GCM:
    case AEAD::ID::AES_256_GCM:
    case AEAD::ID::CHACHA20_POLY1305:
      return 16;

    default:
      throw std::runtime_error("Unsupported algorithm");
  }
}

#if WITH_BORINGSSL
static const EVP_AEAD*
boringssl_cipher(AEAD::ID cipher)
{
  switch (cipher) {
    case AEAD::ID::AES_128_GCM:
      return EVP_aead_aes_128_gcm();

    case AEAD::ID::AES_256_GCM:
      return EVP_aead_aes_256_gcm();

    case AEAD::ID::CHACHA20_POLY1305:
      return EVP_aead_chacha20_poly1305();

    default:
      throw std::runtime_error("Unsupported algorithm");
  }
}
#else
static const EVP_CIPHER*
openssl_cipher(AEAD::ID cipher)
{
  switch (cipher) {
    case AEAD::ID::AES_128_GCM:
      return EVP_aes_128_gcm();

    case AEAD::ID::AES_256_GCM:
      return EVP_aes_256_gcm();

    case AEAD::ID::CHACHA20_POLY1305:
      return EVP_chacha20_poly1305();

    default:
      throw std::runtime_error("Unsupported algorithm");
  }
}
#endif // WITH_BORINGSSL

AEADCipher::AEADCipher(AEAD::ID id_in)
  : AEAD(id_in, cipher_key_size(id_in), cipher_nonce_size(id_in))
  , tag_size(cipher_tag_size(id))
{
}

bytes
AEADCipher::seal(const bytes& key,
                 const bytes& nonce,
                 const bytes& aad,
                 const bytes& pt) const
{
#if WITH_BORINGSSL
  auto ctx = make_typed_unique(
    EVP_AEAD_CTX_new(boringssl_cipher(id), key.data(), key.size(), tag_size));
  if (ctx == nullptr) {
    throw openssl_error();
  }

  auto ct = bytes(pt.size() + tag_size);
  auto out_len = ct.size();
  if (1 != EVP_AEAD_CTX_seal(ctx.get(),
                             ct.data(),
                             &out_len,
                             ct.size(),
                             nonce.data(),
                             nonce.size(),
                             pt.data(),
                             pt.size(),
                             aad.data(),
                             aad.size())) {
    throw openssl_error();
  }

  return ct;
#else
  auto ctx = make_typed_unique(EVP_CIPHER_CTX_new());
  if (ctx == nullptr) {
    throw openssl_error();
  }

  const auto* cipher = openssl_cipher(id);
  if (1 != EVP_EncryptInit(ctx.get(), cipher, key.data(), nonce.data())) {
    throw openssl_error();
  }

  int outlen = 0;
  if (!aad.empty()) {
    if (1 != EVP_EncryptUpdate(ctx.get(),
                               nullptr,
                               &outlen,
                               aad.data(),
                               static_cast<int>(aad.size()))) {
      throw openssl_error();
    }
  }

  bytes ct(pt.size());
  if (1 != EVP_EncryptUpdate(ctx.get(),
                             ct.data(),
                             &outlen,
                             pt.data(),
                             static_cast<int>(pt.size()))) {
    throw openssl_error();
  }

  // Providing nullptr as an argument is safe here because this
  // function never writes with GCM; it only computes the tag
  if (1 != EVP_EncryptFinal(ctx.get(), nullptr, &outlen)) {
    throw openssl_error();
  }

  bytes tag(tag_size);
  if (1 != EVP_CIPHER_CTX_ctrl(ctx.get(),
                               EVP_CTRL_GCM_GET_TAG,
                               static_cast<int>(tag_size),
                               tag.data())) {
    throw openssl_error();
  }

  ct += tag;
  return ct;
#endif // WITH_BORINGSSL
}

std::optional<bytes>
AEADCipher::open(const bytes& key,
                 const bytes& nonce,
                 const bytes& aad,
                 const bytes& ct) const
{
  if (ct.size() < tag_size) {
    throw std::runtime_error("AEAD ciphertext smaller than tag size");
  }

#if WITH_BORINGSSL
  auto ctx = make_typed_unique(EVP_AEAD_CTX_new(
    boringssl_cipher(id), key.data(), key.size(), cipher_tag_size(id)));
  if (ctx == nullptr) {
    throw openssl_error();
  }

  auto pt = bytes(ct.size() - tag_size);
  auto out_len = pt.size();
  if (1 != EVP_AEAD_CTX_open(ctx.get(),
                             pt.data(),
                             &out_len,
                             pt.size(),
                             nonce.data(),
                             nonce.size(),
                             ct.data(),
                             ct.size(),
                             aad.data(),
                             aad.size())) {
    throw openssl_error();
  }

  return pt;
#else
  auto ctx = make_typed_unique(EVP_CIPHER_CTX_new());
  if (ctx == nullptr) {
    throw openssl_error();
  }

  const auto* cipher = openssl_cipher(id);
  if (1 != EVP_DecryptInit(ctx.get(), cipher, key.data(), nonce.data())) {
    throw openssl_error();
  }

  auto inner_ct_size = ct.size() - tag_size;
  auto tag = ct.slice(inner_ct_size, ct.size());
  if (1 != EVP_CIPHER_CTX_ctrl(ctx.get(),
                               EVP_CTRL_GCM_SET_TAG,
                               static_cast<int>(tag_size),
                               tag.data())) {
    throw openssl_error();
  }

  int out_size = 0;
  if (!aad.empty()) {
    if (1 != EVP_DecryptUpdate(ctx.get(),
                               nullptr,
                               &out_size,
                               aad.data(),
                               static_cast<int>(aad.size()))) {
      throw openssl_error();
    }
  }

  bytes pt(inner_ct_size);
  if (1 != EVP_DecryptUpdate(ctx.get(),
                             pt.data(),
                             &out_size,
                             ct.data(),
                             static_cast<int>(inner_ct_size))) {
    throw openssl_error();
  }

  // Providing nullptr as an argument is safe here because this
  // function never writes with GCM; it only verifies the tag
  if (1 != EVP_DecryptFinal(ctx.get(), nullptr, &out_size)) {
    throw std::runtime_error("AEAD authentication failure");
  }

  return pt;
#endif // WITH_BORINGSSL
}

} // namespace mlspp::hpke
