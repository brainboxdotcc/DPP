/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * SPDX-License-Identifier: Apache-2.0
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
#include <dpp/exception.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <string>
#pragma once

namespace dpp::detail {

/**
 * @brief This class wraps a raw SSL_CTX pointer, managing moving,
 * creation, and RAII destruction.
 */
struct wrapped_ssl_ctx {

	/**
	 * @brief SSL_CTX pointer, raw C pointer nastiness
	 */
	SSL_CTX *context{nullptr};

	/**
	 * @brief Get last SSL error message
	 * @return SSL error message
	 */
	std::string get_ssl_error() {
		unsigned long error_code = ERR_get_error();
		if (error_code == 0) {
			return "No error";
		}
		char error_buffer[1024]{0};
		ERR_error_string_n(error_code, error_buffer, sizeof(error_buffer));
		return std::string(error_buffer);
	}

	/**
	 * @brief Create a wrapped SSL context
	 * @param is_server true to create a server context, false to create a client context
	 * @throws dpp::connection_exception if context could not be created
	 */
	explicit wrapped_ssl_ctx(bool is_server = false) : context(SSL_CTX_new(is_server ? TLS_server_method() : TLS_client_method())) {
		if (context == nullptr) {
			throw dpp::connection_exception(err_ssl_context, "Failed to create SSL client context: " + get_ssl_error());
		}
	}

	/**
	 * @brief Copy constructor
	 * @note Intentionally deleted
	 */
	wrapped_ssl_ctx(const wrapped_ssl_ctx&) = delete;

	/**
	 * @brief Copy assignment operator
	 * @note Intentionally deleted
	 */
	wrapped_ssl_ctx& operator=(const wrapped_ssl_ctx&) = delete;

	/**
	 * @brief Move constructor
	 * @param other source context
	 */
	wrapped_ssl_ctx(wrapped_ssl_ctx&& other) noexcept : context(other.context) {
		other.context = nullptr;
	}

	/**
	 * @brief Move assignment operator
	 * @param other source context
	 * @return self
	 */
	wrapped_ssl_ctx& operator=(wrapped_ssl_ctx&& other) noexcept {
		if (this != &other) {
			/* Free current context if any and transfer ownership */
			SSL_CTX_free(context);
			context = other.context;
			other.context = nullptr;
		}
		return *this;
	}

	~wrapped_ssl_ctx() {
		SSL_CTX_free(context);
	}
};

};
