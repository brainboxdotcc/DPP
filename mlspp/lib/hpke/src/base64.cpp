#include <hpke/base64.h>

#include "openssl_common.h"

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>

namespace mlspp::hpke {

std::string
to_base64(const bytes& data)
{
  if (data.empty()) {
    return "";
  }

#if WITH_BORINGSSL
  const auto data_size = data.size();
#else
  const auto data_size = static_cast<int>(data.size());
#endif

  // base64 encoding produces 4 characters for every 3 input bytes (rounded up)
  const auto out_size = (data_size + 2) / 3 * 4;
  auto out = bytes(out_size + 1); // NUL terminator

  const auto result = EVP_EncodeBlock(out.data(), data.data(), data_size);
  if (result != out_size) {
    throw openssl_error();
  }

  out.resize(out.size() - 1); // strip NUL terminator
  return to_ascii(out);
}

std::string
to_base64url(const bytes& data)
{
  if (data.empty()) {
    return "";
  }

  auto encoded = to_base64(data);

  auto pad_start = encoded.find_first_of('=');
  if (pad_start != std::string::npos) {
    encoded = encoded.substr(0, pad_start);
  }

  std::replace(encoded.begin(), encoded.end(), '+', '-');
  std::replace(encoded.begin(), encoded.end(), '/', '_');

  return encoded;
}

bytes
from_base64(const std::string& enc)
{
  if (enc.length() == 0) {
    return {};
  }

  if (enc.length() % 4 != 0) {
    throw std::runtime_error("Base64 length is not divisible by 4");
  }

  const auto in = from_ascii(enc);
  const auto in_size = static_cast<int>(in.size());
  const auto out_size = in_size / 4 * 3;
  auto out = bytes(out_size);

  const auto result = EVP_DecodeBlock(out.data(), in.data(), in_size);
  if (result != out_size) {
    throw openssl_error();
  }

  if (enc.substr(enc.length() - 2, enc.length()) == "==") {
    out.resize(out.size() - 2);
  } else if (enc.substr(enc.length() - 1, enc.length()) == "=") {
    out.resize(out.size() - 1);
  }

  return out;
}

bytes
from_base64url(const std::string& enc)
{
  if (enc.empty()) {
    return {};
  }

  auto enc_copy = enc;
  std::replace(enc_copy.begin(), enc_copy.end(), '-', '+');
  std::replace(enc_copy.begin(), enc_copy.end(), '_', '/');

  while (enc_copy.length() % 4 != 0) {
    enc_copy += "=";
  }

  return from_base64(enc_copy);
}

} // namespace mlspp::hpke
