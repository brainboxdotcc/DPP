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

#if defined _MSC_VER || defined __GNU__ || defined __clang__
#include <immintrin.h>
using avx_512_float = __m512;
using avx_512_int = __m512i;
using avx_2_float = __m256;
using avx_2_int = __m256i;
using avx_float = __m128;
using avx_int = __m128i;

/*
 * @brief Extracts a 32-bit integer from a 128it AVX2 register.
 * @param value The AVX2 register containing packed 16-bit integers.
 * @param index The index of the 32-bit integer to extract (0-3).
 * @return The extracted 32-bit integer.
 */
inline int32_t extract_int32_from_avx(const avx_int& value, int64_t index) {
	switch (index) {
	case 0: {
		return _mm_extract_epi32(value, 0);
	}
	case 1: {
		return _mm_extract_epi32(value, 1);
	}
	case 2: {
		return _mm_extract_epi32(value, 2);
	}
	case 3: {
		return _mm_extract_epi32(value, 3);
	}
	default: {
		return _mm_extract_epi32(value, 0);
	}
	}
}

/*
 * @brief Extracts a 32-bit integer from a 256-bit AVX2 register.
 * @param value The AVX2 register containing packed 32-bit integers.
 * @param index The index of the 32bit integer to extract (0-7).
 * @return The extracted 32-bit integer.
 */
inline int32_t extract_int32_from_avx2(const avx_2_int& value, int64_t index) {
	switch (index) {
	case 0: {
		return _mm256_extract_epi32(value, 0);
	}
	case 1: {
		return _mm256_extract_epi32(value, 1);
	}
	case 2: {
		return _mm256_extract_epi32(value, 2);
	}
	case 3: {
		return _mm256_extract_epi32(value, 3);
	}
	case 4: {
		return _mm256_extract_epi32(value, 4);
	}
	case 5: {
		return _mm256_extract_epi32(value, 5);
	}
	case 6: {
		return _mm256_extract_epi32(value, 6);
	}
	case 7: {
		return _mm256_extract_epi32(value, 7);
	}
	default: {
		return _mm256_extract_epi32(value, 0);
	}
	}
}

/*
 * @brief Extracts a 32-bit integer from a 512-bit AVX-512 register.
 * @param value The AVX-512 register containing packed 16-bit integers.
 * @param index The index of the 32-bit integer to extract (0-15).
 * @return The extracted 32-bit integer.
 */
inline int32_t extract_int32_from_avx512(const avx_512_int& value, int64_t index) {
	alignas(64) int32_t result[32];
	_mm512_store_si512(result, value);
	return result[index];
}
#endif

#ifdef max
	#undef max
#endif
#ifdef min
	#undef min
#endif

namespace dpp {

#ifdef T_AVX512

/**
 * @brief A class for audio mixing operations using AVX2 instructions.
 */
class audio_mixer {
public:
	/*
	 * @brief The number of 32-bit values per CPU register.
	 */
	inline static constexpr int32_t byte_blocks_per_register{ 16 };

	/*
	 * @brief Stores values from a 512-bit AVX vector to a storage location.
	 * @tparam avx_type The 512-bit AVX vector type.
	 * @tparam value_type The target value type for storage.
	 * @param values_to_store The 512-bit AVX vector containing values to store.
	 * @param storage_location Pointer to the storage location.
	 */
	template<typename value_type> inline static void store_values(const avx_512_int& values_to_store, value_type* storage_location) {
		for (int64_t x = 0; x < byte_blocks_per_register; ++x) {
			storage_location[x] = static_cast<value_type>(extract_int32_from_avx512(values_to_store, x));
		}
	}

	/**
	 * @brief Specialization for gathering non-float values into an AVX register.
	 * @tparam avx_type The AVX type to be used (AVX, AVX2, etc.).
	 * @tparam value_type The type of values being gathered.
	 * @tparam Indices Parameter pack of indices for gathering values.
	 * @return An AVX register containing gathered values.
	 */
	template<typename value_type> inline static avx_512_float gather_values(value_type* values) {
		float newArray[byte_blocks_per_register]{};
		for (size_t x = 0; x < byte_blocks_per_register; ++x) {
			newArray[x] = static_cast<float>(values[x]);
		}
		return _mm512_loadu_ps(newArray);
	}

	/**
	 * @brief Collect a single register worth of data from data_in, apply gain and increment, and store the result in data_out.
	 *        This version uses AVX2 instructions.
	 *
	 * @param data_in Pointer to the input array of int32_t values.
	 * @param data_out Pointer to the output array of int16_t values.
	 * @param current_gain The gain to be applied to the elements.
	 * @param increment The increment value to be added to each element.
	 */
	inline static void collect_single_register(int32_t* data_in, int16_t* data_out, float current_gain, float increment) {
		avx_512_float current_samples_new{ _mm512_mul_ps(gather_values(data_in),
			_mm512_add_ps(_mm512_set1_ps(current_gain),
				_mm512_mul_ps(_mm512_set1_ps(increment),
					_mm512_set_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f)))) };

		current_samples_new = _mm512_mask_blend_ps(_mm512_cmp_ps_mask(current_samples_new, _mm512_set1_ps(0.0f), _CMP_GE_OQ),
			_mm512_max_ps(current_samples_new, _mm512_set1_ps(static_cast<float>(std::numeric_limits<int16_t>::min()))),
			_mm512_min_ps(current_samples_new, _mm512_set1_ps(static_cast<float>(std::numeric_limits<int16_t>::max()))));

		store_values(_mm512_cvtps_epi32(current_samples_new), data_out);
	}

	/**
	 * @brief Combine a register worth of elements from decoded_data and store the result in up_sampled_vector.
	 *        This version uses AVX instructions.
	 *
	 * @param up_sampled_vector Pointer to the array of int32_t values.
	 * @param decoded_data Pointer to the array of int16_t values.
	 * @param x Index to select a specific set of elements to combine.
	 */
	inline static void combine_samples(int32_t* up_sampled_vector, const int16_t* decoded_data) {
		auto newValues{ _mm512_cvtps_epi32(_mm512_add_ps(gather_values(up_sampled_vector), gather_values(decoded_data))) };
		store_values(newValues, up_sampled_vector);
	}
};

#elif T_AVX2

/**
 * @brief A class for audio mixing operations using AVX2 instructions.
 */
class audio_mixer {
public:
	/*
	 * @brief The number of 32-bit values per CPU register.
	 */
	inline static constexpr int32_t byte_blocks_per_register{ 8 };

	/*
	 * @brief Stores values from a 256-bit AVX vector to a storage location.
	 * @tparam avx_type The 256-bit AVX vector type.
	 * @tparam value_type The target value type for storage.
	 * @param values_to_store The 256-bit AVX vector containing values to store.
	 * @param storage_location Pointer to the storage location.
	 */
	template<typename value_type> inline static void store_values(const avx_2_int& values_to_store, value_type* storage_location) {
		for (int64_t x = 0; x < byte_blocks_per_register; ++x) {
			storage_location[x] = static_cast<value_type>(extract_int32_from_avx2(values_to_store, x));
		}
	}

	/**
	 * @brief Specialization for gathering non-float values into an AVX register.
	 * @tparam avx_type The AVX type to be used (AVX, AVX2, etc.).
	 * @tparam value_type The type of values being gathered.
	 * @tparam Indices Parameter pack of indices for gathering values.
	 * @return An AVX register containing gathered values.
	 */
	template<typename value_type> inline static avx_2_float gather_values(value_type* values) {
		float newArray[byte_blocks_per_register]{};
		for (size_t x = 0; x < byte_blocks_per_register; ++x) {
			newArray[x] = static_cast<float>(values[x]);
		}
		return _mm256_loadu_ps(newArray);
	}

	/**
	 * @brief Collect a single register worth of data from data_in, apply gain and increment, and store the result in data_out.
	 *        This version uses AVX2 instructions.
	 *
	 * @param data_in Pointer to the input array of int32_t values.
	 * @param data_out Pointer to the output array of int16_t values.
	 * @param current_gain The gain to be applied to the elements.
	 * @param increment The increment value to be added to each element.
	 */
	inline static void collect_single_register(int32_t* data_in, int16_t* data_out, float current_gain, float increment) {
		avx_2_float current_samples_new{ _mm256_mul_ps(gather_values(data_in),
			_mm256_add_ps(_mm256_set1_ps(current_gain),
				_mm256_mul_ps(_mm256_set1_ps(increment), _mm256_set_ps(0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f)))) };

		current_samples_new =
			_mm256_blendv_ps(_mm256_max_ps(current_samples_new, _mm256_set1_ps(static_cast<float>(std::numeric_limits<int16_t>::min()))),
				_mm256_min_ps(current_samples_new, _mm256_set1_ps(static_cast<float>(std::numeric_limits<int16_t>::max()))),
				_mm256_cmp_ps(current_samples_new, _mm256_set1_ps(0.0f), _CMP_GE_OQ));

		store_values(_mm256_cvtps_epi32(current_samples_new), data_out);
	}

	/**
	 * @brief Combine a register worth of elements from decoded_data and store the result in up_sampled_vector.
	 *        This version uses AVX instructions.
	 *
	 * @param up_sampled_vector Pointer to the array of int32_t values.
	 * @param decoded_data Pointer to the array of int16_t values.
	 * @param x Index to select a specific set of elements to combine.
	 */
	inline static void combine_samples(int32_t* up_sampled_vector, const int16_t* decoded_data) {
		auto newValues{ _mm256_cvtps_epi32(_mm256_add_ps(gather_values(up_sampled_vector), gather_values(decoded_data))) };
		store_values(newValues, up_sampled_vector);
	}
};

#elif T_AVX

/**
 * @brief A class for audio mixing operations using AVX2 instructions.
 */
class audio_mixer {
public:
	/*
	 * @brief The number of 32-bit values per CPU register.
	 */
	inline static constexpr int32_t byte_blocks_per_register{ 4 };

	/*
	 * @brief Stores values from a 128-bit AVX vector to a storage location.
	 * @tparam avx_type The 128-bit AVX vector type.
	 * @tparam value_type The target value type for storage.
	 * @param values_to_store The 128-bit AVX vector containing values to store.
	 * @param storage_location Pointer to the storage location.
	 */
	template<typename value_type> inline static void store_values(const avx_int& values_to_store, value_type* storage_location) {
		for (int64_t x = 0; x < byte_blocks_per_register; ++x) {
			storage_location[x] = static_cast<value_type>(extract_int32_from_avx(values_to_store, x));
		}
	}

	/**
	 * @brief Specialization for gathering non-float values into an AVX register.
	 * @tparam avx_type The AVX type to be used (AVX, AVX2, etc.).
	 * @tparam value_type The type of values being gathered.
	 * @tparam Indices Parameter pack of indices for gathering values.
	 * @return An AVX register containing gathered values.
	 */
	template<typename value_type> inline static avx_float gather_values(value_type* values) {
		float newArray[byte_blocks_per_register]{};
		for (size_t x = 0; x < byte_blocks_per_register; ++x) {
			newArray[x] = static_cast<float>(values[x]);
		}
		return _mm_loadu_ps(newArray);
	}

	/**
	 * @brief Collect a single register worth of data from data_in, apply gain and increment, and store the result in data_out.
	 *        This version uses AVX2 instructions.
	 *
	 * @param data_in Pointer to the input array of int32_t values.
	 * @param data_out Pointer to the output array of int16_t values.
	 * @param current_gain The gain to be applied to the elements.
	 * @param increment The increment value to be added to each element.
	 */
	inline static void collect_single_register(int32_t* data_in, int16_t* data_out, float current_gain, float increment) {
		avx_float current_samples_new{ _mm_mul_ps(gather_values(data_in),
			_mm_add_ps(_mm_set1_ps(current_gain), _mm_mul_ps(_mm_set1_ps(increment), _mm_set_ps(0.0f, 1.0f, 2.0f, 3.0f)))) };

		current_samples_new = _mm_blendv_ps(_mm_max_ps(current_samples_new, _mm_set1_ps(static_cast<float>(std::numeric_limits<int16_t>::min()))),
			_mm_min_ps(current_samples_new, _mm_set1_ps(static_cast<float>(std::numeric_limits<int16_t>::max()))),
			_mm_cmp_ps(current_samples_new, _mm_set1_ps(0.0f), _CMP_GE_OQ));

		store_values(_mm_cvtps_epi32(current_samples_new), data_out);
	}

	/**
	 * @brief Combine a register worth of elements from decoded_data and store the result in up_sampled_vector.
	 *        This version uses AVX instructions.
	 *
	 * @param up_sampled_vector Pointer to the array of int32_t values.
	 * @param decoded_data Pointer to the array of int16_t values.
	 * @param x Index to select a specific set of elements to combine.
	 */
	inline static void combine_samples(int32_t* up_sampled_vector, const int16_t* decoded_data) {
		auto newValues{ _mm_cvtps_epi32(_mm_add_ps(gather_values(up_sampled_vector), gather_values(decoded_data))) };
		store_values(newValues, up_sampled_vector);
	}
};

#else 

/**
 * @brief A class for audio mixing operations using AVX instructions.
 */
class audio_mixer {
public:
	/*
	 * @brief The number of 32-bit values per CPU register.
	 */
	inline static constexpr int32_t byte_blocks_per_register{ 2 };

	/**
	 * @brief Collect a single register worth of data from data_in, apply gain and increment, and store the result in data_out.
	 *        This version uses x64 instructions.
	 *
	 * @param data_in Pointer to the input array of int32_t values.
	 * @param data_out Pointer to the output array of int16_t values.
	 * @param current_gain The gain to be applied to the elements.
	 * @param increment The increment value to be added to each element.
	 */
	inline static void collect_single_register(int32_t* data_in, int16_t* data_out, float current_gain, float increment) {
		for (uint64_t x = 0; x < byte_blocks_per_register; ++x) {
			auto increment_neww = increment * x;
			auto current_gain_new = current_gain + increment_neww;
			auto current_sample_new = data_in[x] * current_gain_new;
			if (current_sample_new >= std::numeric_limits<int16_t>::max()) {
				current_sample_new = std::numeric_limits<int16_t>::max();
			}
			else if (current_sample_new <= std::numeric_limits<int16_t>::min()) {
				current_sample_new = std::numeric_limits<int16_t>::min();
			}
			data_out[x] = static_cast<int16_t>(current_sample_new);
		}
	}

	/**
	 * @brief Combine a register worth of elements from decoded_data and store the result in up_sampled_vector.
	 *        This version uses  instructions.
	 *
	 * @param up_sampled_vector Pointer to the array of int32_t values.
	 * @param decoded_data Pointer to the array of int16_t values.
	 */
	inline static void combine_samples(int32_t* up_sampled_vector, const int16_t* decoded_data) {
		for (uint64_t x = 0; x < byte_blocks_per_register; ++x) {
			up_sampled_vector[x] += static_cast<int32_t>(decoded_data[x]);
		}

	}
};

/**@}*/

#endif

} // namespace dpp
