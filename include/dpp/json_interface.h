/************************************************************************************
 *
 * D++, A Lightweight C++ library for Discord
 *
 * Copyright 2022 Craig Edwards and D++ contributors 
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
#include <dpp/json_fwd.h>

namespace dpp {
	/**
	 * @brief Represents an interface for an object that can optionally implement functions
	 * for converting to and from nlohmann::json. In the event either parse_from_json() or
	 * build_json() are not implemented and are called, they will throw at runtime.
	 * 
	 * @tparam T Type of class that implements the interface
	 */
	template<typename T>
	struct DPP_EXPORT json_interface {
	protected:
		/**
		 * @brief Destructor is protected - cannot delete json_interface from outside
		 *
		 * Because we declare this, we have to declare all the other special members
		 */
		~json_interface() = default;

	public:
		/**
		 * @brief Default constructor
		 */
		constexpr json_interface() noexcept = default;

		/**
		 * @brief Copy constructor
		 */
		constexpr json_interface(const json_interface&) noexcept = default;

		/**
		 * @brief Move constructor
		 */
		constexpr json_interface(json_interface&&) noexcept = default;

		/**
		 * @brief Copy assignment operator
		 */
		constexpr json_interface &operator=(const json_interface&) noexcept = default;

		/**
		 * @brief Move assignment operator
		 */
		constexpr json_interface &operator=(json_interface&&) noexcept = default;

		/**
			* @brief Convert object from nlohmann::json
			*
			* @param j nlohmann::json object
			* @return T& Reference to self for fluent calling
			*/
		template <typename U = T, typename = decltype(U::fill_from_json)>
		T& fill_from_json(nlohmann::json* j) {
			return static_cast<T*>(this)->fill_from_json(j);
		}

		/**
			* @brief Convert object from nlohmann::json
			*
			* @param j nlohmann::json object
			* @return T& Reference to self for fluent calling
			*/
		template <typename U = T, typename = decltype(U::build_json)>
		std::string build_json(bool with_id = false) const {
			return static_cast<const T*>(this)->build_json(with_id);
		}
	};
} // namespace dpp
