#pragma once

#include <optional>
#include <stdexcept>

#ifdef VARIANT_COMPAT
#include <variant.hpp>
#else
#include <variant>
#endif // VARIANT_COMPAT

namespace mlspp::tls {

namespace var = std;

// In a similar vein, we provide our own safe accessors for std::optional, since
// std::optional::value() is not available on macOS 10.11.
namespace opt {

template<typename T>
T&
get(std::optional<T>& opt)
{
  if (!opt) {
    throw std::runtime_error("bad_optional_access");
  }
  return *opt;
}

template<typename T>
const T&
get(const std::optional<T>& opt)
{
  if (!opt) {
    throw std::runtime_error("bad_optional_access");
  }
  return *opt;
}

template<typename T>
T&&
get(std::optional<T>&& opt)
{
  if (!opt) {
    throw std::runtime_error("bad_optional_access");
  }
  return std::move(*opt);
}

template<typename T>
const T&&
get(const std::optional<T>&& opt)
{
  if (!opt) {
    throw std::runtime_error("bad_optional_access");
  }
  return std::move(*opt);
}

} // namespace opt
} // namespace mlspp::tls
