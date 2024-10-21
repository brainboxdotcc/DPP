#pragma once

#include <string>
#include <tls/tls_syntax.h>
#include <vector>

namespace mlspp::bytes_ns {

struct bytes
{
  // Ensure defaults
  bytes() = default;
  bytes(const bytes&) = default;
  bytes& operator=(const bytes&) = default;
  bytes(bytes&&) = default;
  bytes& operator=(bytes&&) = default;

  // Zeroize on drop
  ~bytes()
  {
    auto ptr = static_cast<volatile uint8_t*>(_data.data());
    std::fill(ptr, ptr + _data.size(), uint8_t(0));
  }

  // Mimic std::vector ctors
  bytes(size_t count, const uint8_t& value = 0)
    : _data(count, value)
  {
  }

  bytes(std::initializer_list<uint8_t> init)
    : _data(init)
  {
  }

  template<size_t N>
  bytes(const std::array<uint8_t, N>& data)
    : _data(data.begin(), data.end())
  {
  }

  // Slice out sub-vectors (to avoid an iterator ctor)
  bytes slice(size_t begin_index, size_t end_index) const
  {
    const auto begin_it = _data.begin() + begin_index;
    const auto end_it = _data.begin() + end_index;
    return std::vector<uint8_t>(begin_it, end_it);
  }

  // Freely convert to/from std::vector
  bytes(const std::vector<uint8_t>& vec)
    : _data(vec)
  {
  }

  bytes(std::vector<uint8_t>&& vec)
    : _data(vec)
  {
  }

  operator const std::vector<uint8_t>&() const { return _data; }
  operator std::vector<uint8_t>&() { return _data; }
  operator std::vector<uint8_t>&&() && { return std::move(_data); }

  const std::vector<uint8_t>& as_vec() const { return _data; }
  std::vector<uint8_t>& as_vec() { return _data; }

  // Pass through methods
  auto data() const { return _data.data(); }
  auto data() { return _data.data(); }

  auto size() const { return _data.size(); }
  auto empty() const { return _data.empty(); }

  auto begin() const { return _data.begin(); }
  auto begin() { return _data.begin(); }

  auto end() const { return _data.end(); }
  auto end() { return _data.end(); }

  const auto& at(size_t pos) const { return _data.at(pos); }
  auto& at(size_t pos) { return _data.at(pos); }

  void resize(size_t count) { _data.resize(count); }
  void reserve(size_t len) { _data.reserve(len); }
  void push_back(uint8_t byte) { _data.push_back(byte); }

  // Equality operators
  bool operator==(const bytes& other) const;
  bool operator!=(const bytes& other) const;

  bool operator==(const std::vector<uint8_t>& other) const;
  bool operator!=(const std::vector<uint8_t>& other) const;

  // Arithmetic operators
  bytes& operator+=(const bytes& other);
  bytes operator+(const bytes& rhs) const;
  bytes operator^(const bytes& rhs) const;

  // Sorting operators (to allow usage as map keys)
  bool operator<(const bytes& rhs) const;

  // Other, external operators
  friend std::ostream& operator<<(std::ostream& out, const bytes& data);
  friend bool operator==(const std::vector<uint8_t>& lhs, const bytes& rhs);
  friend bool operator!=(const std::vector<uint8_t>& lhs, const bytes& rhs);

  // TLS syntax serialization
  TLS_SERIALIZABLE(_data);

private:
  std::vector<uint8_t> _data;
};

std::string
to_ascii(const bytes& data);

bytes
from_ascii(const std::string& ascii);

std::string
to_hex(const bytes& data);

bytes
from_hex(const std::string& hex);

} // namespace mlspp::bytes_ns
