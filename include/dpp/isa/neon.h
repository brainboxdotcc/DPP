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

#if defined _MSC_VER || defined __GNUC__ || defined __clang__

#include <arm_neon.h>
#include <numeric>
#include <cstdint>
#include <limits>

namespace dpp {

	using neon_float = float32x4_t;

	/**
	 * @brief A class for audio mixing operations using ARM NEON instructions.
	 */
	class audio_mixer {
	public:

		/**
		 * @brief The number of 32-bit values per CPU register.
		 */
		inline static constexpr int32_t byte_blocks_per_register{ 4 };

		/**
		 * @brief Collect a single register worth of data from data_in, apply gain and increment, and store the result in data_out.
		 *        This version uses ARM NEON instructions.
		 *
		 * @param data_in Pointer to the input array of int32_t values.
		 * @param data_out Pointer to the output array of int16_t values.
		 * @param current_gain The gain to be applied to the elements.
		 * @param increment The increment value to be added to each element.
		 */
		inline void collect_single_register(int32_t* data_in, int16_t* data_out, float current_gain, float increment) {
			neon_float gathered_values = gather_values(data_in);
			neon_float gain_vector = vdupq_n_f32(current_gain);
			static constexpr float data[4] = { 0.0f, 1.0f, 2.0f, 3.0f };
			neon_float floats = vld1q_f32(data);
			neon_float increment_vector = vmulq_f32(vdupq_n_f32(increment), floats);
			neon_float current_samples_new = vmulq_f32(gathered_values, vaddq_f32(gain_vector, increment_vector));

			// Clamping the values between int16_t min and max
			neon_float min_val = vdupq_n_f32(static_cast<float>(std::numeric_limits<int16_t>::min()));
			neon_float max_val = vdupq_n_f32(static_cast<float>(std::numeric_limits<int16_t>::max()));

			current_samples_new = vmaxq_f32(current_samples_new, min_val);
			current_samples_new = vminq_f32(current_samples_new, max_val);

			store_values(current_samples_new, data_out);
		}

		/**
		 * @brief Combine a register worth of elements from decoded_data and store the result in up_sampled_vector.
		 *        This version uses ARM NEON instructions.
		 *
		 * @param up_sampled_vector Pointer to the array of int32_t values.
		 * @param decoded_data Pointer to the array of int16_t values.
		 */
		inline void combine_samples(int32_t* up_sampled_vector, const int16_t* decoded_data) {
			neon_float up_sampled = gather_values(up_sampled_vector);
			neon_float decoded = gather_values(decoded_data);
			neon_float newValues = vaddq_f32(up_sampled, decoded);
			store_values(newValues, up_sampled_vector);
		}

	protected:
		/**
		 * @brief Array for storing the values to be loaded/stored.
		 */
		alignas(16) float values[byte_blocks_per_register]{};

		/**
		 * @brief Stores values from a 128-bit NEON vector to a storage location.
		 * @tparam value_type The target value type for storage.
		 * @param values_to_store The 128-bit NEON vector containing values to store.
		 * @param storage_location Pointer to the storage location.
		 */
		template<typename value_type> inline void store_values(const neon_float& values_to_store, value_type* storage_location) {
			vst1q_f32(values, values_to_store);
			for (int64_t x = 0; x < byte_blocks_per_register; ++x) {
				storage_location[x] = static_cast<value_type>(values[x]);
			}
		}

		/**
		 * @brief Specialization for gathering non-float values into a NEON register.
		 * @tparam value_type The type of values being gathered.
		 * @return A NEON register containing gathered values.
		 */
		template<typename value_type> inline neon_float gather_values(value_type* values_new) {
			for (uint64_t x = 0; x < byte_blocks_per_register; ++x) {
				values[x] = static_cast<float>(values_new[x]);
			}
			return vld1q_f32(values);
		}
	};

} // namespace dpp

#endif