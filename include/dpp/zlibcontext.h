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
#include <dpp/exception.h>
#include <cstdint>
#include <vector>
#include <memory>

/**
 * @brief Forward declaration for zlib stream type
 */
typedef struct z_stream_s z_stream;

namespace dpp {

/**
 * @brief Size of decompression buffer for zlib compressed traffic
 */
constexpr size_t DECOMP_BUFFER_SIZE = 512 * 1024;

/**
 * @brief This is an opaque class containing zlib library specific structures.
 * This wraps the C pointers needed for zlib with unique_ptr and gives us a nice
 * buffer abstraction so we don't need to wrestle with raw pointers.
 */
class DPP_EXPORT zlibcontext {
public:
	/**
	 * @brief Zlib stream struct. The actual type is defined in zlib.h
	 * so is only defined in the implementation file.
	 */
	z_stream* d_stream{};

	/**
	 * @brief ZLib decompression buffer.
	 * This is automatically set to DECOMP_BUFFER_SIZE bytes when
	 * the class is constructed.
	 */
	std::vector<unsigned char> decomp_buffer{};

	/**
	 * @brief Total decompressed received bytes counter
	 */
	uint64_t decompressed_total{};

	/**
	 * @brief Initialise zlib struct via inflateInit()
	 * and size the buffer
	 */
	zlibcontext();

	/**
	 * @brief Destroy zlib struct via inflateEnd()
	 */
	~zlibcontext();

	/**
	 * @brief Decompress zlib deflated buffer
	 * @param buffer input compressed stream
	 * @param decompressed output decompressed content
	 * @return an error code on error, or err_no_code_specified (0) on success
	 */
	exception_error_code decompress(const std::string& buffer, std::string& decompressed);
};

}