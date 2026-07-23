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

#include <dpp/restresults.h>

namespace dpp {

confirmation_callback_t::confirmation_callback_t()
	: http_info{}, value{}, bot(nullptr)
{
}

confirmation_callback_t::confirmation_callback_t(const confirmation_callback_t &) = default;
confirmation_callback_t::confirmation_callback_t(confirmation_callback_t &&) noexcept = default;
confirmation_callback_t &confirmation_callback_t::operator=(const confirmation_callback_t &) = default;
confirmation_callback_t &confirmation_callback_t::operator=(confirmation_callback_t &&) noexcept = default;
confirmation_callback_t::~confirmation_callback_t() = default;

}
