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
#pragma once
#include <dpp/export.h>
#include <string>
#include <cstdint>

namespace dpp::detail {

struct wrapped_ssl_ctx;

/**
 * @brief Generate a new wrapped SSL context.
 * If an SSL context already exists for the given port number, it will be returned, else a new one will be
 * generated and cached. Contexts with port = 0 will be considered client contexts. There can only be one
 * client context at a time and it covers all SSL client connections. There can be many SSL server contexts,
 * individual ones can be cached per-port, each with their own loaded SSL private and public key PEM certificate.
 *
 * @param port Port number. Pass zero to create or get the client context.
 * @param private_key Private key PEM pathname for server contexts
 * @param public_key Public key PEM pathname for server contexts
 * @return wrapped SSL context
 */
DPP_EXPORT wrapped_ssl_ctx* generate_ssl_context(uint16_t port = 0, const std::string &private_key = "", const std::string &public_key = "");

/**
 * @brief Release an SSL context
 * @warning Only do this if you are certain no SSL connections remain that use this context.
 * As OpenSSL is a C library it is impossible for us to track this on its behalf. Be careful!
 * @param port port number to release
 */
DPP_EXPORT void release_ssl_context(uint16_t port = 0);

};
