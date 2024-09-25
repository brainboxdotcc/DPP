#include <tls/tls_syntax.h>

// NOLINTNEXTLINE(llvmlibc-implementation-in-namespace)
namespace mlspp::tls {

void
ostream::write_raw(const std::vector<uint8_t>& bytes)
{
  // Not sure what the default argument is here
  _buffer.insert(_buffer.end(), bytes.begin(), bytes.end());
}

// Primitive type writers
ostream&
ostream::write_uint(uint64_t value, int length)
{
  for (int i = length - 1; i >= 0; --i) {
    _buffer.push_back(static_cast<uint8_t>(value >> unsigned(8 * i)));
  }
  return *this;
}

ostream&
operator<<(ostream& out, bool data)
{
  if (data) {
    return out << uint8_t(1);
  }

  return out << uint8_t(0);
}

ostream&
operator<<(ostream& out, uint8_t data) // NOLINT(llvmlibc-callee-namespace)
{
  return out.write_uint(data, 1);
}

ostream&
operator<<(ostream& out, uint16_t data)
{
  return out.write_uint(data, 2);
}

ostream&
operator<<(ostream& out, uint32_t data)
{
  return out.write_uint(data, 4);
}

ostream&
operator<<(ostream& out, uint64_t data)
{
  return out.write_uint(data, 8);
}

// Because pop_back() on an empty vector is undefined
uint8_t
istream::next()
{
  if (_buffer.empty()) {
    throw ReadError("Attempt to read from empty buffer");
  }

  const uint8_t value = _buffer.back();
  _buffer.pop_back();
  return value;
}

// Primitive type readers

istream&
operator>>(istream& in, bool& data)
{
  uint8_t val = 0;
  in >> val;

  // Linter thinks uint8_t is signed (?)
  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  if ((val & 0xFE) != 0) {
    throw ReadError("Malformed boolean");
  }

  data = (val == 1);
  return in;
}

istream&
operator>>(istream& in, uint8_t& data) // NOLINT(llvmlibc-callee-namespace)
{
  return in.read_uint(data, 1);
}

istream&
operator>>(istream& in, uint16_t& data)
{
  return in.read_uint(data, 2);
}

istream&
operator>>(istream& in, uint32_t& data)
{
  return in.read_uint(data, 4);
}

istream&
operator>>(istream& in, uint64_t& data)
{
  return in.read_uint(data, 8);
}

// Varint encoding
static constexpr size_t VARINT_HEADER_OFFSET = 6;
static constexpr uint64_t VARINT_1_HEADER = 0x00;       // 0 << V1_OFFSET
static constexpr uint64_t VARINT_2_HEADER = 0x4000;     // 1 << V2_OFFSET
static constexpr uint64_t VARINT_4_HEADER = 0x80000000; // 2 << V4_OFFSET
static constexpr uint64_t VARINT_1_MAX = 0x3f;
static constexpr uint64_t VARINT_2_MAX = 0x3fff;
static constexpr uint64_t VARINT_4_MAX = 0x3fffffff;

ostream&
varint::encode(ostream& str, const uint64_t& val)
{
  if (val <= VARINT_1_MAX) {
    return str.write_uint(VARINT_1_HEADER | val, 1);
  }

  if (val <= VARINT_2_MAX) {
    return str.write_uint(VARINT_2_HEADER | val, 2);
  }

  if (val <= VARINT_4_MAX) {
    return str.write_uint(VARINT_4_HEADER | val, 4);
  }

  throw WriteError("Varint value exceeds maximum size");
}

istream&
varint::decode(istream& str, uint64_t& val)
{
  auto log_size = size_t(str._buffer.back() >> VARINT_HEADER_OFFSET);
  if (log_size > 2) {
    throw ReadError("Malformed varint header");
  }

  auto read = uint64_t(0);
  auto read_bytes = size_t(size_t(1) << log_size);
  str.read_uint(read, read_bytes);

  switch (log_size) {
    case 0:
      read ^= VARINT_1_HEADER;
      break;

    case 1:
      read ^= VARINT_2_HEADER;
      if (read <= VARINT_1_MAX) {
        throw ReadError("Non-minimal varint");
      }
      break;

    case 2:
      read ^= VARINT_4_HEADER;
      if (read <= VARINT_2_MAX) {
        throw ReadError("Non-minimal varint");
      }
      break;

    default:
      throw ReadError("Malformed varint header");
  }

  val = read;
  return str;
}

} // namespace mlspp::tls
