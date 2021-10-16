/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * Copyright 2021 Craig Edwards and D++ contributors 
 * (https://github.com/brainboxdotcc/DPP/graphs/contributors)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ************************************************************************************/
#include <dpp/etf.h>
#include <dpp/sysdep.h>
#include <dpp/discordevents.h>
#include <dpp/discord.h>
#include <dpp/nlohmann/json.hpp>

#define FORMAT_VERSION 131
#define NEW_FLOAT_EXT 'F'      // 70  [Float64:IEEE float]
#define BIT_BINARY_EXT 'M'     // 77  [UInt32:Len, UInt8:Bits, Len:Data]
#define COMPRESSED 'P'         // 80  [UInt4:UncompressedSize, N:ZlibCompressedData]
#define SMALL_INTEGER_EXT 'a'  // 97  [UInt8:Int]
#define INTEGER_EXT 'b'        // 98  [Int32:Int]
#define FLOAT_EXT 'c'          // 99  [31:Float String] Float in string format (formatted "%.20e", sscanf "%lf"). Superseded by NEW_FLOAT_EXT
#define ATOM_EXT 'd'           // 100 [UInt16:Len, Len:AtomName] max Len is 255
#define REFERENCE_EXT 'e'      // 101 [atom:Node, UInt32:ID, UInt8:Creation]
#define PORT_EXT 'f'           // 102 [atom:Node, UInt32:ID, UInt8:Creation]
#define PID_EXT 'g'            // 103 [atom:Node, UInt32:ID, UInt32:Serial, UInt8:Creation]
#define SMALL_TUPLE_EXT 'h'    // 104 [UInt8:Arity, N:Elements]
#define LARGE_TUPLE_EXT 'i'    // 105 [UInt32:Arity, N:Elements]
#define NIL_EXT 'j'            // 106 empty list
#define STRING_EXT 'k'         // 107 [UInt16:Len, Len:Characters]
#define LIST_EXT 'l'           // 108 [UInt32:Len, Elements, Tail]
#define BINARY_EXT 'm'         // 109 [UInt32:Len, Len:Data]
#define SMALL_BIG_EXT 'n'      // 110 [UInt8:n, UInt8:Sign, n:nums]
#define LARGE_BIG_EXT 'o'      // 111 [UInt32:n, UInt8:Sign, n:nums]
#define NEW_FUN_EXT 'p'        // 112 [UInt32:Size, UInt8:Arity, 16*Uint6-MD5:Uniq, UInt32:Index, UInt32:NumFree, atom:Module, int:OldIndex, int:OldUniq, pid:Pid, NunFree*ext:FreeVars]
#define EXPORT_EXT 'q'         // 113 [atom:Module, atom:Function, smallint:Arity]
#define NEW_REFERENCE_EXT 'r'  // 114 [UInt16:Len, atom:Node, UInt8:Creation, Len*UInt32:ID]
#define SMALL_ATOM_EXT 's'     // 115 [UInt8:Len, Len:AtomName]
#define MAP_EXT 't'            // 116 [UInt32:Airty, N:Pairs]
#define FUN_EXT 'u'            // 117 [UInt4:NumFree, pid:Pid, atom:Module, int:Index, int:Uniq, NumFree*ext:FreeVars]
#define ATOM_UTF8_EXT 'v'      // 118 [UInt16:Len, Len:AtomName] max Len is 255 characters (up to 4 bytes per)
#define SMALL_ATOM_UTF8_EXT 'w' // 119 [UInt8:Len, Len:AtomName]

namespace dpp {

using json = nlohmann::json;

int etf_parser::erlpack_buffer_write(erlpack_buffer *pk, const char *bytes, size_t l) {
	char *buf = pk->buf;
	size_t allocated_size = pk->allocated_size;
	size_t length = pk->length;

	if (length + l > allocated_size) {
		// Grow buffer 2x to avoid excessive re-allocations.
		allocated_size = (length + l) * 2;
		buf = (char *)realloc(buf, allocated_size);

		if (!buf)
			return -1;
	}

	memcpy(buf + length, bytes, l);
	length += l;

	pk->buf = buf;
	pk->allocated_size = allocated_size;
	pk->length = length;
	return 0;
}

int etf_parser::erlpack_append_version(erlpack_buffer *b) {
	static unsigned char buf[1] = {FORMAT_VERSION};
	erlpack_append(b, buf, 1);
}

int etf_parser::erlpack_append_nil(erlpack_buffer *b) {
	static unsigned char buf[5] = {SMALL_ATOM_EXT, 3, 'n', 'i', 'l'};
	erlpack_append(b, buf, 5);
}

int etf_parser::erlpack_append_false(erlpack_buffer *b) {
	static unsigned char buf[7] = {SMALL_ATOM_EXT, 5, 'f', 'a', 'l', 's', 'e'};
	erlpack_append(b, buf, 7);
}

int etf_parser::erlpack_append_true(erlpack_buffer *b) {
	static unsigned char buf[6] = {SMALL_ATOM_EXT, 4, 't', 'r', 'u', 'e'};
	erlpack_append(b, buf, 6);
}

int etf_parser::erlpack_append_small_integer(erlpack_buffer *b, unsigned char d) {
	unsigned char buf[2] = {SMALL_INTEGER_EXT, d};
	erlpack_append(b, buf, 2);
}

int etf_parser::erlpack_append_integer(erlpack_buffer *b, int32_t d) {
	unsigned char buf[5];
	buf[0] = INTEGER_EXT;
	_erlpack_store32(buf + 1, d);
	erlpack_append(b, buf, 5);
}

int etf_parser::erlpack_append_unsigned_long_long(erlpack_buffer *b, unsigned long long d) {
	unsigned char buf[1 + 2 + sizeof(unsigned long long)];
	buf[0] = SMALL_BIG_EXT;

	unsigned char bytes_enc = 0;
	while (d > 0) {
		buf[3 + bytes_enc] = d & 0xFF;
		d >>= 8;
		bytes_enc++;
	}
	buf[1] = bytes_enc;
	buf[2] = 0;

	erlpack_append(b, buf, 1 + 2 + bytes_enc);
}

int etf_parser::erlpack_append_long_long(erlpack_buffer *b, long long d) {
	unsigned char buf[1 + 2 + sizeof(unsigned long long)];
	buf[0] = SMALL_BIG_EXT;
	buf[2] = d < 0 ? 1 : 0;
	unsigned long long ull = d < 0 ? -d : d;
	unsigned char bytes_enc = 0;
	while (ull > 0) {
		buf[3 + bytes_enc] = ull & 0xFF;
		ull >>= 8;
		bytes_enc++;
	}
	buf[1] = bytes_enc;
	erlpack_append(b, buf, 1 + 2 + bytes_enc);
}

int etf_parser::erlpack_append_double(erlpack_buffer *b, double f) {
	unsigned char buf[1 + 8] = {0};
	buf[0] = NEW_FLOAT_EXT;
	type_punner p;
	p.df = f;
	_erlpack_store64(buf + 1, p.ui64);
	erlpack_append(b, buf, 1 + 8);
}

int etf_parser::erlpack_append_atom(erlpack_buffer *b, const char *bytes, size_t size) {
	if (size < 255) {
		unsigned char buf[2] = {SMALL_ATOM_EXT, (unsigned char)size};
		int ret = erlpack_buffer_write(b, (const char *)buf, 2);
		if (ret < 0)
		return ret;

		erlpack_append(b, bytes, size);
	} else {
		unsigned char buf[3];
		buf[0] = ATOM_EXT;

		if (size > 0xFFFF) {
			return 1;
		}

		_erlpack_store16(buf + 1, size);

		int ret = erlpack_buffer_write(b, (const char *)buf, 3);
		if (ret < 0)
		return ret;

		erlpack_append(b, bytes, size);
	}
}

int etf_parser::erlpack_append_atom_utf8(erlpack_buffer *b, const char *bytes, size_t size) {
	if (size < 255) {
		unsigned char buf[2] = {SMALL_ATOM_UTF8_EXT, (unsigned char)size};
		int ret = erlpack_buffer_write(b, (const char *)buf, 2);
		if (ret < 0)
			return ret;

		erlpack_append(b, bytes, size);
	} else {
		unsigned char buf[3];
		buf[0] = ATOM_UTF8_EXT;

		if (size > 0xFFFF) {
			return 1;
		}

		_erlpack_store16(buf + 1, size);

		int ret = erlpack_buffer_write(b, (const char *)buf, 3);
		if (ret < 0)
			return ret;

		erlpack_append(b, bytes, size);
	}
}

int etf_parser::erlpack_append_binary(erlpack_buffer *b, const char *bytes, size_t size) {
  unsigned char buf[5];
  buf[0] = BINARY_EXT;

  _erlpack_store32(buf + 1, size);

  int ret = erlpack_buffer_write(b, (const char *)buf, 5);
  if (ret < 0)
    return ret;

  erlpack_append(b, bytes, size);
}

int etf_parser::erlpack_append_string(erlpack_buffer *b, const char *bytes, size_t size) {
	unsigned char buf[3];
	buf[0] = STRING_EXT;

	_erlpack_store16(buf + 1, size);

	int ret = erlpack_buffer_write(b, (const char *)buf, 3);
	if (ret < 0)
		return ret;

	erlpack_append(b, bytes, size);
}

int etf_parser::erlpack_append_tuple_header(erlpack_buffer *b, size_t size) {
	if (size < 256) {
		unsigned char buf[2];
		buf[0] = SMALL_TUPLE_EXT;
		buf[1] = (unsigned char)size;
		erlpack_append(b, buf, 2);
	} else {
		unsigned char buf[5];
		buf[0] = LARGE_TUPLE_EXT;
		_erlpack_store32(buf + 1, size);
		erlpack_append(b, buf, 5);
	}
}

int etf_parser::erlpack_append_nil_ext(erlpack_buffer *b) {
	static unsigned char buf[1] = {NIL_EXT};
	erlpack_append(b, buf, 1);
}

int etf_parser::erlpack_append_list_header(erlpack_buffer *b, size_t size) {
	unsigned char buf[5];
	buf[0] = LIST_EXT;
	_erlpack_store32(buf + 1, size);
	erlpack_append(b, buf, 5);
}

int etf_parser::erlpack_append_map_header(erlpack_buffer *b, size_t size) {
	unsigned char buf[5];
	buf[0] = MAP_EXT;
	_erlpack_store32(buf + 1, size);
	erlpack_append(b, buf, 5);
}

etf_parser::etf_parser()
{
}

etf_parser::~etf_parser() = default;


uint8_t etf_parser::read8() {
	if (offset + sizeof(uint8_t) > size) {
		return 0;
	}
	auto val = *reinterpret_cast<const uint8_t*>(data + offset);
	offset += sizeof(uint8_t);
	return val;
}

uint16_t etf_parser::read16() {
	if (offset + sizeof(uint16_t) > size) {
		return 0;
	}
	uint16_t val = _erlpack_be16(*reinterpret_cast<const uint16_t*>(data + offset));
	offset += sizeof(uint16_t);
	return val;
}

uint32_t etf_parser::read32() {
	if (offset + sizeof(uint32_t) > size) {
		return 0;
	}
	uint32_t val = _erlpack_be32(*reinterpret_cast<const uint32_t*>(data + offset));
	offset += sizeof(uint32_t);
	return val;
}

uint64_t etf_parser::read64() {
	if (offset + sizeof(uint64_t) > size) {
		return 0;
	}
	uint64_t val = _erlpack_be64(*reinterpret_cast<const uint64_t*>(data + offset));
	offset += sizeof(val);
	return val;
}

void etf_parser::inner_parse(const std::string& data, nlohmann::json& j) {
}

void etf_parser::parse(const std::string& in, nlohmann::json& j) {
	offset = 0;
	size = in.size();
	data = (char*)in.data();
	inner_parse(in, j);
}

std::string etf_parser::build(const nlohmann::json& j) {
	return "";
}

};

