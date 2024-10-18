#include <bytes/bytes.h>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace mlspp::bytes_ns {

bool
bytes::operator==(const bytes& other) const
{
  return *this == other._data;
}

bool
bytes::operator!=(const bytes& other) const
{
  return !(*this == other._data);
}

bool
bytes::operator==(const std::vector<uint8_t>& other) const
{
  const size_t size = other.size();
  if (_data.size() != size) {
    return false;
  }

  unsigned char diff = 0;
  for (size_t i = 0; i < size; ++i) {
    // Not sure why the linter thinks `diff` is signed
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    diff |= (_data.at(i) ^ other.at(i));
  }
  return (diff == 0);
}

bool
bytes::operator!=(const std::vector<uint8_t>& other) const
{
  return !(*this == other);
}

bytes&
bytes::operator+=(const bytes& other)
{
  // Not sure what the default argument is here
  // NOLINTNEXTLINE(fuchsia-default-arguments)
  _data.insert(end(), other.begin(), other.end());
  return *this;
}

bytes
bytes::operator+(const bytes& rhs) const
{
  bytes out = *this;
  out += rhs;
  return out;
}

bool
bytes::operator<(const bytes& rhs) const
{
  return _data < rhs._data;
}

bytes
bytes::operator^(const bytes& rhs) const
{
  if (size() != rhs.size()) {
    throw std::invalid_argument("XOR with unequal size");
  }

  bytes out = *this;
  for (size_t i = 0; i < size(); ++i) {
    out.at(i) ^= rhs.at(i);
  }
  return out;
}

std::string
to_ascii(const bytes& data)
{
  return { data.begin(), data.end() };
}

bytes
from_ascii(const std::string& ascii)
{
  return std::vector<uint8_t>(ascii.begin(), ascii.end());
}

std::string
to_hex(const bytes& data)
{
  std::stringstream hex(std::ios_base::out);
  hex.flags(std::ios::hex);
  for (const auto& byte : data) {
    hex << std::setw(2) << std::setfill('0') << int(byte);
  }
  return hex.str();
}

bytes
from_hex(const std::string& hex)
{
  if (hex.length() % 2 == 1) {
    throw std::invalid_argument("Odd-length hex string");
  }

  auto len = hex.length() / 2;
  auto out = bytes(len);
  for (size_t i = 0; i < len; i += 1) {
    const std::string byte = hex.substr(2 * i, 2);
    out.at(i) = static_cast<uint8_t>(strtol(byte.c_str(), nullptr, 16));
  }

  return out;
}

std::ostream&
operator<<(std::ostream& out, const bytes& data)
{
  // Adjust this threshold to make output more compact
  const size_t threshold = 0xffff;
  if (data.size() < threshold) {
    return out << to_hex(data);
  }

  return out << to_hex(data.slice(0, threshold)) << "...";
}

bool
operator==(const std::vector<uint8_t>& lhs, const bytes_ns::bytes& rhs)
{
  return rhs == lhs;
}

bool
operator!=(const std::vector<uint8_t>& lhs, const bytes_ns::bytes& rhs)
{
  return rhs != lhs;
}

} // namespace mlspp::bytes_ns
