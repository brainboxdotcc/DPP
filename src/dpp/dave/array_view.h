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

#include <vector>

namespace dpp::dave {

template <typename T>
class array_view {
public:
	array_view() = default;
	array_view(T* data, size_t size)
	  : data_(data)
	  , size_(size)
	{
	}

	size_t size() const { return size_; }
	T* data() const { return data_; }

	T* begin() const { return data_; }
	T* end() const { return data_ + size_; }

private:
	T* data_ = nullptr;
	size_t size_ = 0;
};

template <typename T>
inline array_view<T> make_array_view(T* data, size_t size)
{
	return array_view<T>(data, size);
}

template <typename T>
inline array_view<T> make_array_view(std::vector<T>& data)
{
	return array_view<T>(data.data(), data.size());
}

} // namespace dpp::dave

