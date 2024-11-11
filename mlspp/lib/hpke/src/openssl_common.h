#pragma once

#include <hpke/hpke.h>
#include <stdexcept>

namespace mlspp::hpke {

template<typename T>
void
typed_delete(T* ptr);

template<typename T>
using typed_unique_ptr = std::unique_ptr<T, decltype(&typed_delete<T>)>;

template<typename T>
typed_unique_ptr<T>
make_typed_unique(T* ptr)
{
  return typed_unique_ptr<T>(ptr, typed_delete<T>);
}

std::runtime_error
openssl_error();

} // namespace mlspp::hpke
