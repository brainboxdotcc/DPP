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
#define __STDC_FORMAT_MACROS 1
#include <dpp/etf.h>
#include <dpp/sysdep.h>
#include <dpp/discordevents.h>
#include <dpp/discord.h>
#include <dpp/dispatcher.h>
#include <dpp/nlohmann/json.hpp>
#include <zlib.h>
#include <iostream>

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

void etf_parser::erlpack_buffer_write(erlpack_buffer *pk, const char *bytes, size_t l) {

	if (pk->length + l > pk->buf.size()) {
		// Grow buffer 2x to avoid excessive re-allocations.
		pk->buf.resize((pk->length + l) * 2);
	}

	memcpy(pk->buf.data() + pk->length, bytes, l);
	pk->length += l;
}

 void etf_parser::erlpack_append_version(erlpack_buffer *b) {
	static unsigned char buf[1] = {FORMAT_VERSION};
	erlpack_append(b, buf, 1);
}

 void etf_parser::erlpack_append_nil(erlpack_buffer *b) {
	static unsigned char buf[5] = {SMALL_ATOM_EXT, 3, 'n', 'i', 'l'};
	erlpack_append(b, buf, 5);
}

 void etf_parser::erlpack_append_false(erlpack_buffer *b) {
	static unsigned char buf[7] = {SMALL_ATOM_EXT, 5, 'f', 'a', 'l', 's', 'e'};
	erlpack_append(b, buf, 7);
}

 void etf_parser::erlpack_append_true(erlpack_buffer *b) {
	static unsigned char buf[6] = {SMALL_ATOM_EXT, 4, 't', 'r', 'u', 'e'};
	erlpack_append(b, buf, 6);
}

 void etf_parser::erlpack_append_small_integer(erlpack_buffer *b, unsigned char d) {
	unsigned char buf[2] = {SMALL_INTEGER_EXT, d};
	erlpack_append(b, buf, 2);
}

 void etf_parser::erlpack_append_integer(erlpack_buffer *b, int32_t d) {
	unsigned char buf[5];
	buf[0] = INTEGER_EXT;
	_erlpack_store32(buf + 1, d);
	erlpack_append(b, buf, 5);
}

 void etf_parser::erlpack_append_unsigned_long_long(erlpack_buffer *b, unsigned long long d) {
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

 void etf_parser::erlpack_append_long_long(erlpack_buffer *b, long long d) {
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

 void etf_parser::erlpack_append_double(erlpack_buffer *b, double f) {
	unsigned char buf[1 + 8] = {0};
	buf[0] = NEW_FLOAT_EXT;
	type_punner p;
	p.df = f;
	_erlpack_store64(buf + 1, p.ui64);
	erlpack_append(b, buf, 1 + 8);
}

 void etf_parser::erlpack_append_atom(erlpack_buffer *b, const char *bytes, size_t size) {
	if (size < 255) {
		unsigned char buf[2] = {SMALL_ATOM_EXT, (unsigned char)size};
		erlpack_buffer_write(b, (const char *)buf, 2);
		erlpack_append(b, bytes, size);
	} else {
		unsigned char buf[3];
		buf[0] = ATOM_EXT;

		if (size > 0xFFFF) {
			throw dpp::exception("ETF: Atom too large");
		}

		_erlpack_store16(buf + 1, size);
		erlpack_buffer_write(b, (const char *)buf, 3);
		erlpack_append(b, bytes, size);
	}
}

 void etf_parser::erlpack_append_atom_utf8(erlpack_buffer *b, const char *bytes, size_t size) {
	if (size < 255) {
		unsigned char buf[2] = {SMALL_ATOM_UTF8_EXT, (unsigned char)size};
		erlpack_buffer_write(b, (const char *)buf, 2);
		erlpack_append(b, bytes, size);
	} else {
		unsigned char buf[3];
		buf[0] = ATOM_UTF8_EXT;

		if (size > 0xFFFF) {
			throw dpp::exception("ETF: Atom too large");
		}

		_erlpack_store16(buf + 1, size);
		erlpack_buffer_write(b, (const char *)buf, 3);
		erlpack_append(b, bytes, size);
	}
}

void etf_parser::erlpack_append_binary(erlpack_buffer *b, const char *bytes, size_t size) {
	unsigned char buf[5];
	buf[0] = BINARY_EXT;

	_erlpack_store32(buf + 1, size);
	erlpack_buffer_write(b, (const char *)buf, 5);
	erlpack_append(b, bytes, size);
}

 void etf_parser::erlpack_append_string(erlpack_buffer *b, const char *bytes, size_t size) {
	unsigned char buf[3];
	buf[0] = STRING_EXT;

	_erlpack_store16(buf + 1, size);
	erlpack_buffer_write(b, (const char *)buf, 3);
	erlpack_append(b, bytes, size);
}

 void etf_parser::erlpack_append_tuple_header(erlpack_buffer *b, size_t size) {
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

 void etf_parser::erlpack_append_nil_ext(erlpack_buffer *b) {
	static unsigned char buf[1] = {NIL_EXT};
	erlpack_append(b, buf, 1);
}

 void etf_parser::erlpack_append_list_header(erlpack_buffer *b, size_t size) {
	unsigned char buf[5];
	buf[0] = LIST_EXT;
	_erlpack_store32(buf + 1, size);
	erlpack_append(b, buf, 5);
}

 void etf_parser::erlpack_append_map_header(erlpack_buffer *b, size_t size) {
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
		throw dpp::exception("ETF: read8() past end of buffer");
	}
	auto val = *reinterpret_cast<const uint8_t*>(data + offset);
	offset += sizeof(uint8_t);
	return val;
}

uint16_t etf_parser::read16() {
	if (offset + sizeof(uint16_t) > size) {
		throw dpp::exception("ETF: read16() past end of buffer");
	}
	uint16_t val = _erlpack_be16(*reinterpret_cast<const uint16_t*>(data + offset));
	offset += sizeof(uint16_t);
	return val;
}

uint32_t etf_parser::read32() {
	if (offset + sizeof(uint32_t) > size) {
		throw dpp::exception("ETF: read32() past end of buffer");
	}
	uint32_t val = _erlpack_be32(*reinterpret_cast<const uint32_t*>(data + offset));
	offset += sizeof(uint32_t);
	return val;
}

uint64_t etf_parser::read64() {
	if (offset + sizeof(uint64_t) > size) {
		throw dpp::exception("ETF: read64() past end of buffer");
	}
	uint64_t val = _erlpack_be64(*reinterpret_cast<const uint64_t*>(data + offset));
	offset += sizeof(val);
	return val;
}

const char* etf_parser::readString(uint32_t length) {
	if (offset + length > size) {
		return nullptr;
	}

	const uint8_t* str = data + offset;
	offset += length;
	return (const char*)str;
}

json etf_parser::processAtom(const char* atom, uint16_t length) {
	if (atom == NULL) {
		return json();
	}

	json j;

	if (length >= 3 && length <= 5) {
		if (length == 3 && strncmp(atom, "nil", 3) == 0) {
			return j;
		}
		else if (length == 4 && strncmp(atom, "null", 4) == 0) {
			return j;
		}
		else if(length == 4 && strncmp(atom, "true", 4) == 0) {
			j = true;
			return j;
		}
		else if (length == 5 && strncmp(atom, "false", 5) == 0) {
			j = false;
			return j;
		}
	}

	j = std::string(atom, length);
	return j;
}

json etf_parser::decodeAtom() {
	auto length = read16();
	const char* atom = readString(length);
	return processAtom(atom, length);
}

json etf_parser::decodeSmallAtom() {
	auto length = read8();
	const char* atom = readString(length);
	return processAtom(atom, length);
}

json etf_parser::decodeSmallInteger() {
	json j;
	j = (int8_t)read8();
	return j;
}

json etf_parser::decodeInteger() {
	json j;
	j = (int32_t)read32();
	return j;
}

json etf_parser::decodeArray(uint32_t length) {
	json array = json::array();
	for(uint32_t i = 0; i < length; ++i) {
		array.push_back(inner_parse());
	}
	return array;
}

json etf_parser::decodeList() {
	const uint32_t length = read32();
	auto array = decodeArray(length);

	const auto tailMarker = read8();
	if (tailMarker != NIL_EXT) {
		return json();
	}

	return array;
}

json etf_parser::decodeTuple(uint32_t length) {
	return decodeArray(length);
}

json etf_parser::decodeNil() {
	return json::array();
}

json etf_parser::decodeMap() {
	const uint32_t length = read32();
	auto map = json::object();
	for(uint32_t i = 0; i < length; ++i) {
		const auto key = inner_parse();
		const auto value = inner_parse();
		if (key.is_number()) {
			map[std::to_string(key.get<uint64_t>())] = value;
		} else {
			map[key.get<std::string>()] = value;
		}
	}
	return map;
}

json etf_parser::decodeFloat() {

	const uint8_t FLOAT_LENGTH = 31;
	const char* floatStr = readString(FLOAT_LENGTH);

	if (floatStr == NULL) {
		return json();
	}

	double number;
	char nullTerimated[FLOAT_LENGTH + 1] = {0};

	memcpy(nullTerimated, floatStr, FLOAT_LENGTH);

	auto count = sscanf(nullTerimated, "%lf", &number);

	if (count != 1) {
		return json();
	}

	json j = number;
	return j;
}

json etf_parser::decodeNewFloat() {
	union {
		uint64_t ui64;
		double df;
	} val;
	val.ui64 = read64();
	json j = val.df;
	return j;
}

json etf_parser::decodeBig(uint32_t digits) {
	const uint8_t sign = read8();

	if (digits > 8) {
		throw dpp::exception("ETF: big integer larger than 8 bytes unsupported");
	}

	uint64_t value = 0;
	uint64_t b = 1;
	for(uint32_t i = 0; i < digits; ++i) {
		uint64_t digit = read8();
		value += digit * b;
		b <<= 8;
	}

	if (digits <= 4) {
		if (sign == 0) {
			json j = std::to_string(static_cast<uint32_t>(value));
			return j;
		}

		const bool isSignBitAvailable = (value & (1 << 31)) == 0;
		if (isSignBitAvailable) {
			int32_t negativeValue = -static_cast<int32_t>(value);
			json j = std::to_string(negativeValue);
			return j;
		}
	}

	char outBuffer[32] = {0}; // 9223372036854775807
	const char* const formatString = sign == 0 ? "%llu" : "-%ll";
	const int res = sprintf(outBuffer, formatString, value);

	if (res < 0) {
		throw dpp::exception("Decode big integer failed");
	}
	const uint8_t length = (uint8_t)res;
	json j = std::string(outBuffer, length);
	return j;
}

json etf_parser::decodeSmallBig() {
	const auto bytes = read8();
	return decodeBig(bytes);
}

json etf_parser::decodeLargeBig() {
	const auto bytes = read32();
	return decodeBig(bytes);
}

json etf_parser::decodeBinaryAsString() {
	const auto length = read32();
	const char* str = readString(length);
	if (str == NULL) {
		return json();
	}
	std::string s = std::string(str, length);
	json j = std::string(str, length);
	return j;
}

json etf_parser::decodeString() {
	const auto length = read16();
	const char* str = readString(length);
	if (str == NULL) {
		return json();
	}
	json j = std::string(str, length);
	return j;
}

json etf_parser::decodeStringAsList() {
	const auto length = read16();
	json array = json::array();
	if (offset + length > size) {
		throw dpp::exception("String list past end of buffer");
	}
	for(uint16_t i = 0; i < length; ++i) {
		array.push_back(decodeSmallInteger());
	}
	return array;
}

json etf_parser::decodeSmallTuple() {
	return decodeTuple(read8());
}

json etf_parser::decodeLargeTuple() {
	return decodeTuple(read32());
}

json etf_parser::decodeCompressed() {
	const uint32_t uncompressedSize = read32();
	unsigned long sourceSize = uncompressedSize;
	std::vector<uint8_t> outBuffer;
	outBuffer.reserve(uncompressedSize);
	const int ret = uncompress((Bytef*)outBuffer.data(), &sourceSize, (const unsigned char*)(data + offset), (uLong)(size - offset));

	offset += sourceSize;
	if (ret != Z_OK) {
		throw dpp::exception("ETF compressed value: decompresson error");
	}

	uint8_t* old_data = data;
	size_t old_size = size;
	size_t old_offset = offset;
	data = outBuffer.data();
	size = uncompressedSize;
	offset = 0;
	json j = inner_parse();
	data = old_data;
	size = old_size;
	offset = old_offset;
	return j;
}

json etf_parser::decodeReference() {
	json reference;

	reference["node"] = inner_parse();

	std::vector<int32_t> ids;
	ids.push_back(read32());
	reference["id"] = ids;

	reference["creation"] = read8();

	return reference;
}

json etf_parser::decodeNewReference() {
	json reference;

	uint16_t len = read16();
	reference["node"] = inner_parse();
	reference["creation"] = read8();

	std::vector<int32_t> ids;
	for(uint16_t i = 0; i < len; ++i) {
		ids.push_back(read32());
	}
	reference["id"] = ids;

	return reference;
}

json etf_parser::decodePort() {
	json port;
	port["node"] = inner_parse();
	port["id"] = read32();
	port["creation"] = read8();
	return port;
}

json etf_parser::decodePID() {
	json pid;
	pid["node"] = inner_parse();
	pid["id"] = read32();
	pid["serial"] = read32();
	pid["creation"] = read8();
	return pid;
}

json etf_parser::decodeExport() {
	json exp;
	exp["mod"] = inner_parse();
	exp["fun"] = inner_parse();
	exp["arity"] = inner_parse();
	return exp;
}

json etf_parser::inner_parse() {
	if(offset >= size) {
		throw dpp::exception("Read past end of ETF buffer");
	}

	const uint8_t type = read8();

	switch(type) {
		case SMALL_INTEGER_EXT:
			return decodeSmallInteger();
		break;
		case INTEGER_EXT:
			return decodeInteger();
		break;
		case FLOAT_EXT:
			return decodeFloat();
		break;
		case NEW_FLOAT_EXT:
			return decodeNewFloat();
		break;
		case ATOM_EXT:
			return decodeAtom();
		break;
		case SMALL_ATOM_EXT:
			return decodeSmallAtom();
		break;
		case SMALL_TUPLE_EXT:
			return decodeSmallTuple();
		break;
		case LARGE_TUPLE_EXT:
			return decodeLargeTuple();
		break;
		case NIL_EXT:
			return decodeNil();
		break;
		case STRING_EXT:
			return decodeStringAsList();
		break;
		case LIST_EXT:
			return decodeList();
		break;
		case MAP_EXT:
			return decodeMap();
		break;
		case BINARY_EXT:
			return decodeBinaryAsString();
		break;
		case SMALL_BIG_EXT:
			return decodeSmallBig();
		break;
		case LARGE_BIG_EXT:
			return decodeLargeBig();
		break;
		case REFERENCE_EXT:
			return decodeReference();
		break;
		case NEW_REFERENCE_EXT:
			return decodeNewReference();
		break;
		case PORT_EXT:
			return decodePort();
		break;
		case PID_EXT:
			return decodePID();
		break;
		case EXPORT_EXT:
			return decodeExport();
		break;
		case COMPRESSED:
			return decodeCompressed();
		break;
		default:
			throw dpp::exception("Unknown data type in ETF");
		break;
	}
}

json etf_parser::parse(const std::string& in) {
	offset = 0;
	size = in.size();
	data = (uint8_t*)in.data();
	const auto version = read8();
	if (version == FORMAT_VERSION) {
		return inner_parse();
	} else {
		throw dpp::exception("Incorrect ETF version");
	}
}

void etf_parser::inner_build(const json* i, erlpack_buffer* b)
{
	if (i->is_number_integer()) {
		int number = i->get<int>();
		if (number >= 0 && number <= 127) {
			unsigned char num = (unsigned char)number;
			erlpack_append_small_integer(b, num);
		}
		/*else if (i->is_number_unsigned()) {
			auto uNum = (uint64_t)i->get<uint64_t>();
			erlpack_append_unsigned_long_long(b, uNum);
		}*/
		else if (i->is_number_integer()) {
			erlpack_append_integer(b, number);
		}
	}
	else if (i->is_number_float()) {
		double decimal = i->get<double>();
		erlpack_append_double(b, decimal);
	}
	else if (i->is_null()) {
		erlpack_append_nil(b);
	}
	else if (i->is_boolean()) {
		bool truthy = i->get<bool>();
		if (truthy) {
			erlpack_append_true(b);
		} else {
			erlpack_append_false(b);
		}
	}
	else if (i->is_string()) {
		const std::string s = i->get<std::string>();
		erlpack_append_binary(b, s.c_str(), s.length());
	}
	else if (i->is_array()) {
		const size_t length = i->size();
		if (length == 0) {
			erlpack_append_nil_ext(b);
		} else {
			if (length > std::numeric_limits<uint32_t>::max() - 1) {
				throw dpp::exception("ETF encode: List too large for ETF");
			}
		}

		erlpack_append_list_header(b, length);

		for(size_t index = 0; index < length; ++index) {
			inner_build(&((*i)[index]), b);
		}
		erlpack_append_nil_ext(b);
	}
	else if (i->is_object()) {
		const size_t length = i->size();
		if (length > std::numeric_limits<uint32_t>::max() - 1) {
			throw dpp::exception("ETF encode: Map too large for ETF");
		}
		erlpack_append_map_header(b, length);
		for (auto n = i->begin(); n != i->end(); ++n) {
			json jstr = n.key();
			inner_build(&jstr, b);
			inner_build(&(n.value()), b);
		}
	}
}

std::string etf_parser::build(const json& j) {
	erlpack_buffer pk(1024 * 1024);
	erlpack_append_version(&pk);
	inner_build(&j, &pk);
	return std::string(pk.buf.data(), pk.length);
}

erlpack_buffer::erlpack_buffer(size_t initial) {
	buf.resize(initial);
	length = 0;
}

erlpack_buffer::~erlpack_buffer() = default;

};

