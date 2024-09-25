#pragma once

#include <optional>
#include <stdexcept>

#ifdef VARIANT_COMPAT
#include <variant.hpp>
#else
#include <variant>
#endif // VARIANT_COMPAT

namespace mlspp::tls {

// To balance backward-compatibility with macOS 10.11 with forward-compatibility
// with future versions of C++, we use `mpark::variant` or `std::variant` as
// needed, using `var::variant` to refer to whichever one is in use.
#ifdef VARIANT_COMPAT
namespace var = mpark;
#else
namespace var = std;
#endif // VARIANT_COMPAT

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
