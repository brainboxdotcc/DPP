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
 * This folder is a modified fork of libdave, https://github.com/discord/libdave
 * Copyright (c) 2024 Discord, Licensed under MIT
 *
 ************************************************************************************/
#pragma once

#include <string>
#include <mls/credential.h>
#include "version.h"

namespace dpp::dave::mls {

/**
 * @brief Create user credentials
 * @param user_id user id
 * @param version protocol version
 * @return
 */
::mlspp::Credential create_user_credential(const std::string& user_id, protocol_version version);

/**
 * @brief Convert user credentials to string
 * @param cred user credentials
 * @param version protocol version
 * @return user credentials as string
 */
std::string user_credential_to_string(const ::mlspp::Credential& cred, protocol_version version);

}

