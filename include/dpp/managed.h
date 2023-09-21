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
#include <dpp/snowflake.h>
#include <string>

namespace dpp {

	class managed_base {};

	/** @brief The managed class is the base class for various types that can
	 * be stored in a cache that are identified by a dpp::snowflake id.
	 */
	template<typename derived_type> class managed : public managed_base {
	public:
		/*
		* @brief Default constructor.
		*/
		inline managed() noexcept = default;

		/**
		 * @brief Constructor, initialises ID
		 * @param nid ID to set
		 */
		inline managed(const snowflake nid) {
			static_cast<derived_type*>(this)->id = nid;
		}

		/**
		 * @brief Get the creation time of this object according to Discord.
		 * 
		 * @return double creation time inferred from the snowflake ID.
		 * The minimum possible value is the first second of 2015.
		 */
		inline double get_creation_time() const {
			return static_cast<const derived_type*>(this)->id.get_creation_time();
		}

		/**
		 * @brief Comparison operator for comparing two managed objects by id
		 * 
		 * @param other Other object to compare against
		 * @return true objects are the same id
		 * @return false objects are not the same id
		 */
		inline bool operator==(const managed& other) const noexcept {
			return static_cast<const derived_type*>(this)->id == static_cast<const derived_type*>(&other)->id;
		}

		/**
		 * @brief Comparison operator for comparing two managed objects by id
		 * 
		 * @param other Other object to compare against
		 * @return true objects are not the same id
		 * @return false objects are the same id
		 */
		inline bool operator!=(const managed& other) const noexcept {
			return static_cast<const derived_type*>(this)->id != static_cast<const derived_type*>(&other)->id;
		}
	};

} // namespace dpp
