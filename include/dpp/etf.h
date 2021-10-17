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

#pragma once
#include <dpp/export.h>
#include <dpp/discord.h>
#include <dpp/nlohmann/json.hpp>

namespace dpp {

union type_punner {
	uint64_t ui64;
	double df;
};

struct DPP_EXPORT erlpack_buffer {
	std::vector<char> buf;
	size_t length;

	erlpack_buffer(size_t initial);
	~erlpack_buffer();
};

#define erlpack_append(pk, buf, len) erlpack_buffer_write(pk, (const char *)buf, len)

/**
 * @brief The etf_parser class can serialise and deserialise ETF (Erlang Term Format)
 * into and out of an nlohmann::json object, so that layers above the websocket don't
 * have to be any different for handling ETF.
 */
class DPP_EXPORT etf_parser {

	size_t size;

	size_t offset;

	uint8_t* data;

	nlohmann::json inner_parse();

	uint8_t read8();

	uint16_t read16();

	uint32_t read32();

	uint64_t read64();

	const char* readString(uint32_t length);

	nlohmann::json processAtom(const char* atom, uint16_t length);
	nlohmann::json decodeAtom();
	nlohmann::json decodeSmallAtom();
	nlohmann::json decodeSmallInteger();
	nlohmann::json decodeInteger();
	nlohmann::json decodeArray(uint32_t length);
	nlohmann::json decodeList();
	nlohmann::json decodeTuple(uint32_t length);
	nlohmann::json decodeNil();
	nlohmann::json decodeMap();
	nlohmann::json decodeFloat();
	nlohmann::json decodeNewFloat();
	nlohmann::json decodeBig(uint32_t digits);
	nlohmann::json decodeSmallBig();
	nlohmann::json decodeLargeBig();
	nlohmann::json decodeBinaryAsString();
	nlohmann::json decodeString();
	nlohmann::json decodeStringAsList();
	nlohmann::json decodeSmallTuple();
	nlohmann::json decodeLargeTuple();
	nlohmann::json decodeCompressed();
	nlohmann::json decodeReference();
	nlohmann::json decodeNewReference();
	nlohmann::json decodePort();
	nlohmann::json decodePID();
	nlohmann::json decodeExport();

	void erlpack_buffer_write(erlpack_buffer *pk, const char *bytes, size_t l);

	void erlpack_append_version(erlpack_buffer *b);

	void erlpack_append_nil(erlpack_buffer *b);

	void erlpack_append_false(erlpack_buffer *b);

	void erlpack_append_true(erlpack_buffer *b);

	void erlpack_append_small_integer(erlpack_buffer *b, unsigned char d);

	void erlpack_append_integer(erlpack_buffer *b, int32_t d);

	void erlpack_append_unsigned_long_long(erlpack_buffer *b, unsigned long long d);

	void erlpack_append_long_long(erlpack_buffer *b, long long d);

	void erlpack_append_double(erlpack_buffer *b, double f);

	void erlpack_append_atom(erlpack_buffer *b, const char *bytes, size_t size);

	void erlpack_append_atom_utf8(erlpack_buffer *b, const char *bytes, size_t size);

	void erlpack_append_binary(erlpack_buffer *b, const char *bytes, size_t size);

	void erlpack_append_string(erlpack_buffer *b, const char *bytes, size_t size);

	void erlpack_append_tuple_header(erlpack_buffer *b, size_t size);

	void erlpack_append_nil_ext(erlpack_buffer *b);

	void erlpack_append_list_header(erlpack_buffer *b, size_t size);

	void erlpack_append_map_header(erlpack_buffer *b, size_t size);

	void inner_build(const nlohmann::json* j, erlpack_buffer* b);

public:
	/** Constructor */
	etf_parser();

	/** Destructor */
	~etf_parser();

	nlohmann::json parse(const std::string& in);

	std::string build(const nlohmann::json& j);
};

};
