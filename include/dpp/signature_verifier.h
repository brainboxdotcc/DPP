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
#include <string>
#include <mutex>

namespace dpp {

/**
 * @brief Verifies signatures on incoming webhooks using OpenSSL
 */
class signature_verifier {
public:
	/**
	 * @brief Constructor initializes the OpenSSL context and public key buffer
	 */
	signature_verifier();

	/**
	 * @brief Verifies the signature with the provided public key, timestamp, body, and signature
	 * @param timestamp The timestamp of the request
	 * @param body The body of the request
	 * @param signature The hex-encoded signature to verify
	 * @param public_key_hex The hex-encoded public key
	 * @return true if the signature is valid, false otherwise
	 */
	bool verify_signature(const std::string& timestamp, const std::string& body, const std::string& signature, const std::string& public_key_hex);

};

}

