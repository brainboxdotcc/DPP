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
#include <zlib.h>
#include <memory>
#include <cstring>
#include <dpp/zlibcontext.h>

namespace dpp {

zlibcontext::zlibcontext() : d_stream(new z_stream()) {
	std::memset(d_stream, 0, sizeof(z_stream));
	int error = inflateInit(d_stream);
	if (error != Z_OK) {
		delete d_stream;
		throw dpp::connection_exception((exception_error_code)error, "Can't initialise stream compression!");
	}
	decomp_buffer.resize(DECOMP_BUFFER_SIZE);
}

zlibcontext::~zlibcontext() {
	inflateEnd(d_stream);
	delete d_stream;
}

exception_error_code zlibcontext::decompress(const std::string& buffer, std::string& decompressed) {
	decompressed.clear();
	/* This is safe; zlib requires us to cast away the const. The underlying buffer is unchanged. */
	d_stream->next_in = reinterpret_cast<Bytef*>(const_cast<char*>(buffer.data()));
	d_stream->avail_in = static_cast<uInt>(buffer.size());
	do {
		d_stream->next_out = static_cast<Bytef*>(decomp_buffer.data());
		d_stream->avail_out = DECOMP_BUFFER_SIZE;
		int ret = inflate(d_stream, Z_NO_FLUSH);
		size_t have = DECOMP_BUFFER_SIZE - d_stream->avail_out;
		switch (ret) {
			case Z_NEED_DICT:
			case Z_STREAM_ERROR:
				return err_compression_stream;
			case Z_DATA_ERROR:
				return err_compression_data;
			case Z_MEM_ERROR:
				return err_compression_memory;
			case Z_OK:
				decompressed.append(decomp_buffer.begin(), decomp_buffer.begin() + have);
				decompressed_total += have;
				break;
			default:
				/* Stub */
				break;
		}
	} while (d_stream->avail_out == 0);
	return err_no_code_specified;
}

};