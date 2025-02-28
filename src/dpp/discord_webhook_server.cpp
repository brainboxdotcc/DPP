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
#include <dpp/discord_webhook_server.h>
#include <dpp/signature_verifier.h>
#include <dpp/discordevents.h>
#include <dpp/dispatcher.h>
#include <memory>

namespace dpp {

discord_webhook_server::discord_webhook_server(cluster* owner, const std::string& discord_public_key, const std::string_view address, uint16_t port, const std::string& ssl_private_key, const std::string& ssl_public_key)
 : http_server(owner, address, port, [this](http_server_request* request) { handle_request(request); }, ssl_private_key, ssl_public_key), public_key_hex(discord_public_key)
{
}

void discord_webhook_server::handle_request(http_server_request* request) {
	std::string signature = request->get_header("X-Signature-Ed25519");
	std::string timestamp = request->get_header("X-Signature-Timestamp");
	if (signature.empty() || timestamp.empty()) {
		request->set_status(401).set_response_header("Content-Type", "text/plain").set_response_body("Unsigned requests are not allowed");
		return;
	}
	if (!verifier.verify_signature(timestamp, request->get_request_body(), signature, public_key_hex)) {
		request->set_status(401).set_response_header("Content-Type", "text/plain").set_response_body("Access denied");
		return;
	}

	json j = json::parse(request->get_request_body());
	std::string reply_body = events::internal_handle_interaction(creator, 0, j, request->get_request_body(), true);

	request->set_status(200).set_response_header("Content-Type", "application/json")
		.set_response_body(reply_body);
}

}
