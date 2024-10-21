#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>
#include <vector>

#include <tls/compat.h>

namespace mlspp::tls {

// For indicating no min or max in vector definitions
const size_t none = std::numeric_limits<size_t>::max();

class WriteError : public std::invalid_argument
{
public:
  using parent = std::invalid_argument;
  using parent::parent;
};

class ReadError : public std::invalid_argument
{
public:
  using parent = std::invalid_argument;
  using parent::parent;
};

///
/// Declarations of Streams and Traits
///

class ostream
{
public:
  static const size_t none = std::numeric_limits<size_t>::max();

  void write_raw(const std::vector<uint8_t>& bytes);

  const std::vector<uint8_t>& bytes() const { return _buffer; }
  size_t size() const { return _buffer.size(); }
  bool empty() const { return _buffer.empty(); }

private:
  std::vector<uint8_t> _buffer;
  ostream& write_uint(uint64_t value, int length);

  friend ostream& operator<<(ostream& out, bool data);
  friend ostream& operator<<(ostream& out, uint8_t data);
  friend ostream& operator<<(ostream& out, uint16_t data);
  friend ostream& operator<<(ostream& out, uint32_t data);
  friend ostream& operator<<(ostream& out, uint64_t data);

  template<typename T>
  friend ostream& operator<<(ostream& out, const std::vector<T>& data);

  friend struct varint;
};

class istream
{
public:
  istream(const std::vector<uint8_t>& data)
    : _buffer(data)
  {
    // So that we can use the constant-time pop_back
    std::reverse(_buffer.begin(), _buffer.end());
  }

  size_t size() const { return _buffer.size(); }
  bool empty() const { return _buffer.empty(); }

  std::vector<uint8_t> bytes()
  {
    auto bytes = _buffer;
    std::reverse(bytes.begin(), bytes.end());
    return bytes;
  }

private:
  istream() {}
  std::vector<uint8_t> _buffer;
  uint8_t next();

  template<typename T>
  istream& read_uint(T& data, size_t length)
  {
    uint64_t value = 0;
    for (size_t i = 0; i < length; i += 1) {
      value = (value << unsigned(8)) + next();
    }
    data = static_cast<T>(value);
    return *this;
  }

  friend istream& operator>>(istream& in, bool& data);
  friend istream& operator>>(istream& in, uint8_t& data);
  friend istream& operator>>(istream& in, uint16_t& data);
  friend istream& operator>>(istream& in, uint32_t& data);
  friend istream& operator>>(istream& in, uint64_t& data);

  template<typename T>
  friend istream& operator>>(istream& in, std::vector<T>& data);

  friend struct varint;
};

// Traits must have static encode and decode methods, of the following form:
//
//     static ostream& encode(ostream& str, const T& val);
//     static istream& decode(istream& str, T& val);
//
// Trait types will never be constructed; only these static methods are used.
// The value arguments to encode and decode can be as strict or as loose as
// desired.
//
// Ultimately, all interesting encoding should be done through traits.
//
// * vectors
// * variants
// * varints

struct pass
{
  template<typename T>
  static ostream& encode(ostream& str, const T& val);

  template<typename T>
  static istream& decode(istream& str, T& val);
};

template<typename Ts>
struct variant
{
  template<typename... Tp>
  static inline Ts type(const var::variant<Tp...>& data);

  template<typename... Tp>
  static ostream& encode(ostream& str, const var::variant<Tp...>& data);

  template<size_t I = 0, typename Te, typename... Tp>
  static inline typename std::enable_if<I == sizeof...(Tp), void>::type
  read_variant(istream&, Te, var::variant<Tp...>&);

  template<size_t I = 0, typename Te, typename... Tp>
    static inline typename std::enable_if <
    I<sizeof...(Tp), void>::type read_variant(istream& str,
                                              Te target_type,
                                              var::variant<Tp...>& v);

  template<typename... Tp>
  static istream& decode(istream& str, var::variant<Tp...>& data);
};

struct varint
{
  static ostream& encode(ostream& str, const uint64_t& val);
  static istream& decode(istream& str, uint64_t& val);
};

///
/// Writer implementations
///

// Primitive writers defined in .cpp file

// Array writer
template<typename T, size_t N>
ostream&
operator<<(ostream& out, const std::array<T, N>& data)
{
  for (const auto& item : data) {
    out << item;
  }
  return out;
}

// Optional writer
template<typename T>
ostream&
operator<<(ostream& out, const std::optional<T>& opt)
{
  if (!opt) {
    return out << uint8_t(0);
  }

  return out << uint8_t(1) << opt::get(opt);
}

// Enum writer
template<typename T, std::enable_if_t<std::is_enum<T>::value, int> = 0>
ostream&
operator<<(ostream& str, const T& val)
{
  auto u = static_cast<std::underlying_type_t<T>>(val);
  return str << u;
}

// Vector writer
template<typename T>
ostream&
operator<<(ostream& str, const std::vector<T>& vec)
{
  // Pre-encode contents
  ostream temp;
  for (const auto& item : vec) {
    temp << item;
  }

  // Write the encoded length, then the pre-encoded data
  varint::encode(str, temp._buffer.size());
  str.write_raw(temp.bytes());

  return str;
}

///
/// Reader implementations
///

// Primitive type readers defined in .cpp file

// Array reader
template<typename T, size_t N>
istream&
operator>>(istream& in, std::array<T, N>& data)
{
  for (auto& item : data) {
    in >> item;
  }
  return in;
}

// Optional reader
template<typename T>
istream&
operator>>(istream& in, std::optional<T>& opt)
{
  uint8_t present = 0;
  in >> present;

  switch (present) {
    case 0:
      opt.reset();
      return in;

    case 1:
      opt.emplace();
      return in >> opt::get(opt);

    default:
      throw std::invalid_argument("Malformed optional");
  }
}

// Enum reader
// XXX(rlb): It would be nice if this could enforce that the values are valid,
// but C++ doesn't seem to have that ability.  When used as a tag for variants,
// the variant reader will enforce, at least.
template<typename T, std::enable_if_t<std::is_enum<T>::value, int> = 0>
istream&
operator>>(istream& str, T& val)
{
  std::underlying_type_t<T> u;
  str >> u;
  val = static_cast<T>(u);
  return str;
}

// Vector reader
template<typename T>
istream&
operator>>(istream& str, std::vector<T>& vec)
{
  // Read the encoded data size
  auto size = uint64_t(0);
  varint::decode(str, size);
  if (size > str._buffer.size()) {
    throw ReadError("Vector is longer than remaining data");
  }

  // Read the elements of the vector
  // NB: Remember that we store the vector in reverse order
  // NB: This requires that T be default-constructible
  istream r;
  r._buffer =
    std::vector<uint8_t>{ str._buffer.end() - size, str._buffer.end() };

  vec.clear();
  while (r._buffer.size() > 0) {
    vec.emplace_back();
    r >> vec.back();
  }

  // Truncate the primary buffer
  str._buffer.erase(str._buffer.end() - size, str._buffer.end());

  return str;
}

// Abbreviations
template<typename T>
std::vector<uint8_t>
marshal(const T& value)
{
  ostream w;
  w << value;
  return w.bytes();
}

template<typename T>
void
unmarshal(const std::vector<uint8_t>& data, T& value)
{
  istream r(data);
  r >> value;
}

template<typename T, typename... Tp>
T
get(const std::vector<uint8_t>& data, Tp... args)
{
  T value(args...);
  unmarshal(data, value);
  return value;
}

// Use this macro to define struct serialization with minimal boilerplate
#define TLS_SERIALIZABLE(...)                                                  \
  static const bool _tls_serializable = true;                                  \
  auto _tls_fields_r()                                                         \
  {                                                                            \
    return std::forward_as_tuple(__VA_ARGS__);                                 \
  }                                                                            \
  auto _tls_fields_w() const                                                   \
  {                                                                            \
    return std::forward_as_tuple(__VA_ARGS__);                                 \
  }

// If your struct contains nontrivial members (e.g., vectors), use this to
// define traits for them.
#define TLS_TRAITS(...)                                                        \
  static const bool _tls_has_traits = true;                                    \
  using _tls_traits = std::tuple<__VA_ARGS__>;

template<typename T>
struct is_serializable
{
  template<typename U>
  static std::true_type test(decltype(U::_tls_serializable));

  template<typename U>
  static std::false_type test(...);

  static const bool value = decltype(test<T>(true))::value;
};

template<typename T>
struct has_traits
{
  template<typename U>
  static std::true_type test(decltype(U::_tls_has_traits));

  template<typename U>
  static std::false_type test(...);

  static const bool value = decltype(test<T>(true))::value;
};

///
/// Trait implementations
///

// Pass-through (normal encoding/decoding)
template<typename T>
ostream&
pass::encode(ostream& str, const T& val)
{
  return str << val;
}

template<typename T>
istream&
pass::decode(istream& str, T& val)
{
  return str >> val;
}

// Variant encoding
template<typename Ts, typename Tv>
constexpr Ts
variant_map();

#define TLS_VARIANT_MAP(EnumType, MappedType, enum_value)                      \
  template<>                                                                   \
  constexpr EnumType variant_map<EnumType, MappedType>()                       \
  {                                                                            \
    return EnumType::enum_value;                                               \
  }

template<typename Ts>
template<typename... Tp>
inline Ts
variant<Ts>::type(const var::variant<Tp...>& data)
{
  const auto get_type = [](const auto& v) {
    return variant_map<Ts, std::decay_t<decltype(v)>>();
  };
  return var::visit(get_type, data);
}

template<typename Ts>
template<typename... Tp>
ostream&
variant<Ts>::encode(ostream& str, const var::variant<Tp...>& data)
{
  const auto write_variant = [&str](auto&& v) {
    using Tv = std::decay_t<decltype(v)>;
    str << variant_map<Ts, Tv>() << v;
  };
  var::visit(write_variant, data);
  return str;
}

template<typename Ts>
template<size_t I, typename Te, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
variant<Ts>::read_variant(istream&, Te, var::variant<Tp...>&)
{
  throw ReadError("Invalid variant type label");
}

template<typename Ts>
  template<size_t I, typename Te, typename... Tp>
  inline
  typename std::enable_if < I<sizeof...(Tp), void>::type
                            variant<Ts>::read_variant(istream& str,
                                                      Te target_type,
                                                      var::variant<Tp...>& v)
{
  using Tc = var::variant_alternative_t<I, var::variant<Tp...>>;
  if (variant_map<Ts, Tc>() == target_type) {
    str >> v.template emplace<I>();
    return;
  }

  read_variant<I + 1>(str, target_type, v);
}

template<typename Ts>
template<typename... Tp>
istream&
variant<Ts>::decode(istream& str, var::variant<Tp...>& data)
{
  Ts target_type;
  str >> target_type;
  read_variant(str, target_type, data);
  return str;
}

// Struct writer without traits (enabled by macro)
template<size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
write_tuple(ostream&, const std::tuple<Tp...>&)
{
}

template<size_t I = 0, typename... Tp>
  inline typename std::enable_if <
  I<sizeof...(Tp), void>::type
  write_tuple(ostream& str, const std::tuple<Tp...>& t)
{
  str << std::get<I>(t);
  write_tuple<I + 1, Tp...>(str, t);
}

template<typename T>
inline
  typename std::enable_if<is_serializable<T>::value && !has_traits<T>::value,
                          ostream&>::type
  operator<<(ostream& str, const T& obj)
{
  write_tuple(str, obj._tls_fields_w());
  return str;
}

// Struct writer with traits (enabled by macro)
template<typename Tr, size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
write_tuple_traits(ostream&, const std::tuple<Tp...>&)
{
}

template<typename Tr, size_t I = 0, typename... Tp>
  inline typename std::enable_if <
  I<sizeof...(Tp), void>::type
  write_tuple_traits(ostream& str, const std::tuple<Tp...>& t)
{
  std::tuple_element_t<I, Tr>::encode(str, std::get<I>(t));
  write_tuple_traits<Tr, I + 1, Tp...>(str, t);
}

template<typename T>
inline
  typename std::enable_if<is_serializable<T>::value && has_traits<T>::value,
                          ostream&>::type
  operator<<(ostream& str, const T& obj)
{
  write_tuple_traits<typename T::_tls_traits>(str, obj._tls_fields_w());
  return str;
}

// Struct reader without traits (enabled by macro)
template<size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
read_tuple(istream&, const std::tuple<Tp...>&)
{
}

template<size_t I = 0, typename... Tp>
  inline
  typename std::enable_if < I<sizeof...(Tp), void>::type
                            read_tuple(istream& str, const std::tuple<Tp...>& t)
{
  str >> std::get<I>(t);
  read_tuple<I + 1, Tp...>(str, t);
}

template<typename T>
inline
  typename std::enable_if<is_serializable<T>::value && !has_traits<T>::value,
                          istream&>::type
  operator>>(istream& str, T& obj)
{
  read_tuple(str, obj._tls_fields_r());
  return str;
}

// Struct reader with traits (enabled by macro)
template<typename Tr, size_t I = 0, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type
read_tuple_traits(istream&, const std::tuple<Tp...>&)
{
}

template<typename Tr, size_t I = 0, typename... Tp>
  inline typename std::enable_if <
  I<sizeof...(Tp), void>::type
  read_tuple_traits(istream& str, const std::tuple<Tp...>& t)
{
  std::tuple_element_t<I, Tr>::decode(str, std::get<I>(t));
  read_tuple_traits<Tr, I + 1, Tp...>(str, t);
}

template<typename T>
inline
  typename std::enable_if<is_serializable<T>::value && has_traits<T>::value,
                          istream&>::type
  operator>>(istream& str, T& obj)
{
  read_tuple_traits<typename T::_tls_traits>(str, obj._tls_fields_r());
  return str;
}

} // namespace mlspp::tls
