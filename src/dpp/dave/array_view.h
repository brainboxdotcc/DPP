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

/**
 * @brief This is a simple wrapper around a range of values, e.g. a vector or array.
 * It is only constant if type T is constant.
 * @tparam T Type in array or vector
 */
template <typename T> class array_view {
public:
	/**
	 * @brief Default constructor
	 */
	array_view() = default;

	/**
	 * @brief Construct array_view with data and size of data
	 * @param data data pointer to array
	 * @param size size of array
	 */
	array_view(T* data, size_t size) : array(data), array_size(size) {
	}

	/**
	 * @brief Get size of view
	 * @return size
	 */
	size_t size() const {
		return array_size;
	}

	/**
	 * @brief Get data of view from first element
	 * @return data
	 */
	T* data() const {
		return array;
	}

	/**
	 * @brief Get start of view, first element
	 * @return first element
	 */
	T* begin() const {
		return array;
	}

	/**
	 * @brief Get ending iterator of view, 1+last element
	 * @return end of view
	 */
	T* end() const {
		return array + array_size;
	}

private:
	/**
	 * @brief array data
	 */
	T* array = nullptr;
	/**
	 * @brief Array size
	 */
	size_t array_size = 0;
};

/**
 * @brief Construct new array view from C style array
 * @tparam T array member type
 * @param data pointer to array
 * @param size size of array
 * @return array_view
 */
template <typename T>
inline array_view<T> make_array_view(T* data, size_t size)
{
	return array_view<T>(data, size);
}

/**
 * @brief Construct new array view from vector
 * @tparam T vector member type
 * @param data vector
 * @return array_view
 */
template <typename T>
inline array_view<T> make_array_view(std::vector<T>& data)
{
	return array_view<T>(data.data(), data.size());
}

}
