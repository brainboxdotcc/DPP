#include "openssl_common.h"

#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#if defined(WITH_OPENSSL3)
#include <openssl/param_build.h>
#endif

namespace mlspp::hpke {

template<>
void
typed_delete(EVP_CIPHER_CTX* ptr)
{
  EVP_CIPHER_CTX_free(ptr);
}

#if WITH_BORINGSSL
template<>
void
typed_delete(EVP_AEAD_CTX* ptr)
{
  EVP_AEAD_CTX_free(ptr);
}
#endif

template<>
void
typed_delete(EVP_PKEY_CTX* ptr)
{
  EVP_PKEY_CTX_free(ptr);
}

template<>
void
typed_delete(EVP_MD_CTX* ptr)
{
  EVP_MD_CTX_free(ptr);
}

#if !defined(WITH_OPENSSL3)
template<>
void
typed_delete(HMAC_CTX* ptr)
{
  HMAC_CTX_free(ptr);
}
#endif

template<>
void
typed_delete(EVP_PKEY* ptr)
{
  EVP_PKEY_free(ptr);
}

template<>
void
typed_delete(BIGNUM* ptr)
{
  BN_free(ptr);
}

template<>
void
typed_delete(EC_POINT* ptr)
{
  EC_POINT_free(ptr);
}

#if !defined(WITH_OPENSSL3)
template<>
void
typed_delete(EC_KEY* ptr)
{
  EC_KEY_free(ptr);
}
#endif

#if defined(WITH_OPENSSL3)
template<>
void
typed_delete(EVP_MAC* ptr)
{
  EVP_MAC_free(ptr);
}

template<>
void
typed_delete(EVP_MAC_CTX* ptr)
{
  EVP_MAC_CTX_free(ptr);
}

template<>
void
typed_delete(EC_GROUP* ptr)
{
  EC_GROUP_free(ptr);
}

template<>
void
typed_delete(OSSL_PARAM_BLD* ptr)
{
  OSSL_PARAM_BLD_free(ptr);
}

template<>
void
typed_delete(OSSL_PARAM* ptr)
{
  OSSL_PARAM_free(ptr);
}
#endif

template<>
void
typed_delete(X509* ptr)
{
  X509_free(ptr);
}

template<>
void
typed_delete(STACK_OF(GENERAL_NAME) * ptr)
{
  sk_GENERAL_NAME_pop_free(ptr, GENERAL_NAME_free);
}

template<>
void
typed_delete(BIO* ptr)
{
  BIO_vfree(ptr);
}

template<>
void
typed_delete(ASN1_TIME* ptr)
{
  ASN1_TIME_free(ptr);
}

///
/// Map OpenSSL errors to C++ exceptions
///

std::runtime_error
openssl_error()
{
  auto code = ERR_get_error();
  return std::runtime_error(ERR_error_string(code, nullptr));
}

} // namespace mlspp::hpke
