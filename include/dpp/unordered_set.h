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

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(_WIN64)
#include <memory_resource>
#define ALLOCATOR(T) std::pmr::polymorphic_allocator<T>
#elif defined(__APPLE__) || defined(__linux)
#define ALLOCATOR(T) std::allocator<T>
#endif

#include <shared_mutex>

#ifdef max
#undef max
#endif

#ifdef __has_cpp_attribute
#if __has_cpp_attribute(no_unique_address)
#ifdef _MSC_VER
#define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif
#else
#define NO_UNIQUE_ADDRESS 
#endif 
#else
#define NO_UNIQUE_ADDRESS 
#endif

namespace dpp {

	template<typename OTy = void> struct fnv1a_hash {
		uint64_t operator()(const snowflake& data) const {
			auto new_value = static_cast<OTy>(data);
			return internal_hash_function(reinterpret_cast<const uint8_t*>(&new_value), sizeof(new_value));
		}

		uint64_t operator()(snowflake&& data) const {
			auto new_value = static_cast<OTy>(data);
			return internal_hash_function(reinterpret_cast<const uint8_t*>(&new_value), sizeof(new_value));
		}

	protected:
		static constexpr uint64_t fnv_offset_basis{ 14695981039346656037ull };
		static constexpr uint64_t fnv_prime{ 1099511628211ull };

		size_t internal_hash_function(const uint8_t* value, size_t count) const {
			auto hash = fnv_offset_basis;
			for (size_t x = 0; x < count; ++x) {
				hash ^= value[x];
				hash *= fnv_prime;
			}
			return hash;
		}
	};

	template<typename KTy, typename OTy> struct key_accessor {
		KTy& operator()(const OTy& other) {
			return other->id;
		}

		KTy& operator()(OTy&& other) {
			return other->id;
		}
	};

	template<typename KTy, typename OTy, typename KATy = key_accessor<KTy, OTy>> class memory_core {
	public:
		using key_type = KTy;
		using value_type = OTy;
		using key_accessor = KATy;
		using reference = value_type&;
		using const_reference = const reference;
		using pointer = value_type*;
		using size_type = size_t;
		using key_hasher = fnv1a_hash<key_type>;

		class memory_core_iterator {
		public:
			using value_type = typename memory_core::value_type;
			using reference = typename memory_core::reference;
			using pointer = typename memory_core::pointer;
			using iterator_category = std::forward_iterator_tag;

			inline memory_core_iterator(memory_core* core, size_t index) : core(core), index(index) {
			}

			inline memory_core_iterator& operator++() {
				index++;
				while (index < core->capacity && !core->data[index].operator bool()) {
					index++;
				}
				return *this;
			}

			inline bool operator==(const memory_core_iterator& other) const {
				return core == other.core && index == other.index;
			}

			inline bool operator!=(const memory_core_iterator& other) const {
				return !(*this == other);
			}

			inline reference operator*() const {
				return core->data[index];
			}

		protected:
			memory_core* core;
			size_t index;
		};

		using iterator = memory_core_iterator;

		class sentinel_holder {
		public:
			inline sentinel_holder() noexcept = default;

			inline sentinel_holder& operator=(sentinel_holder&& other) = delete;
			inline sentinel_holder(sentinel_holder&& other) = delete;
			inline sentinel_holder& operator=(const sentinel_holder& other) = delete;
			inline sentinel_holder(const sentinel_holder& other) = delete;

			inline operator bool() const noexcept {
				return is_it_active;
			}

			inline operator reference() noexcept {
				return object;
			}

			inline void activate(value_type&& data) noexcept {
				object = std::forward<value_type>(data);
				is_it_active = true;
			}

			inline value_type&& disable() noexcept {
				is_it_active = false;
				return std::move(object);
			}

		protected:
			bool is_it_active{ false };
			value_type object{};
		};

		using sentinel_allocator = ALLOCATOR(sentinel_holder);

		inline memory_core(size_type new_capacity) : capacity(new_capacity), size(0), data(allocator.allocate(new_capacity)) {
			for (size_t x = 0; x < new_capacity; ++x) {
				allocator.construct(&data[x]);
			}
		};

		inline memory_core& operator=(memory_core&& other) noexcept {
			if (this != &other) {
				pointer* tmp_data = data;
				size_t tmp_capacity = capacity;
				size_t tmp_size = size;
				data = other.data;
				capacity = other.capacity;
				size = other.size;
				other.data = tmp_data;
				other.capacity = tmp_capacity;
				other.size = tmp_size;
			}
			return *this;
		}

		inline memory_core(memory_core&& other) noexcept {
			*this = std::move(other);
		}

		inline memory_core& operator=(const memory_core& other) noexcept {
			if (this != &other) {
				memory_core<key_type, value_type> newData{ other.capacity };
				for (size_t x = 0; x < other.capacity; ++x) {
					if (other.data[x].operator bool()) {
						newData.emplace(other.data[x].disable());
					}
				}
				std::swap(this->capacity, newData.capacity);
				std::swap(this->data, newData.data);
				std::swap(this->size, newData.size);
			}
			return *this;
		}

		inline memory_core(const memory_core& other) noexcept {
			*this = other;
		}

		inline void emplace(value_type&& value) noexcept {
			emplace_internal(std::forward<value_type>(value));
		}

		inline void emplace(const value_type& value) noexcept {
			emplace_internal(value);
		}

		inline iterator find(key_type&& key) noexcept {
			size_type index = key_hasher{}(key) % capacity;
			size_type current_index{};
			while (current_index < capacity) {
				if (data[index].operator bool() && key_accessor {}(data[index].operator reference()) == key) {
					return iterator{ this, index };
				}
				index = (index + 1) % capacity;
				++current_index;
			}

			return end();
		}

		inline iterator find(const key_type& key) noexcept {
			size_type index = key_hasher{}(key) % capacity;
			size_type current_index{};
			while (current_index < capacity) {
				if (data[index].operator bool() && key_accessor {}(data[index].operator reference()) == key) {
					return iterator{ this, index };
				}
				index = (index + 1) % capacity;
				++current_index;
			}

			return end();
		}

		inline bool contains(key_type&& key) noexcept {
			size_type index = key_hasher{}(key) % capacity;
			size_type current_index{};
			while (current_index < capacity) {
				if (data[index].operator bool() && key_accessor {}(data[index].operator reference()) == key) {
					return true;
				}
				index = (index + 1) % capacity;
				++current_index;
			}

			return false;
		}

		inline bool contains(const key_type& key) noexcept {
			size_type index = key_hasher{}(key) % capacity;
			size_type current_index{};
			while (current_index < capacity) {
				if (data[index].operator bool() && key_accessor {}(data[index].operator reference()) == key) {
					return true;
				}
				index = (index + 1) % capacity;
				++current_index;
			}

			return false;
		}

		inline void erase(key_type&& key) noexcept {
			size_type index = key_hasher{}(key) % capacity;
			size_type current_index{};
			while (current_index < capacity) {
				if (data[index].operator bool() && key_accessor {}(data[index].operator reference()) == key) {
					data[index].disable();
					size--;
					if (size < capacity / 4 && capacity > 10) {
						resize(capacity / 2);
					}
					return;
				}
				index = (index + 1) % capacity;
				++current_index;
			}
		}

		inline void erase(const key_type& key) noexcept {
			size_type index = key_hasher{}(key) % capacity;
			size_type current_index{};
			while (current_index < capacity) {
				if (data[index].operator bool() && key_accessor {}(data[index].operator reference()) == key) {
					data[index].disable();
					size--;
					if (size < capacity / 4 && capacity > 10) {
						resize(capacity / 2);
					}
					return;
				}
				index = (index + 1) % capacity;
				++current_index;
			}
		}

		inline iterator begin() noexcept {
			size_type index{};
			while (index < capacity) {
				if (data[index].operator bool()) {
					return iterator{ this, index };
				}
				++index;
			}
			return end();
		}

		inline iterator end() noexcept {
			return iterator{ this, capacity };
		}

		inline iterator begin() const noexcept {
			size_type index{};
			while (index < capacity) {
				if (data[index].operator bool()) {
					return iterator{ this, index };
				}
				++index;
			}
			return end();
		}

		inline iterator end() const noexcept {
			return iterator{ this, capacity };
		}

		inline size_type get_size() const noexcept {
			return size;
		}

		inline bool is_empty() const noexcept {
			return size == 0;
		}

		inline bool is_full() const noexcept {
			return static_cast<float>(size) >= static_cast<float>(capacity) * 0.75f;
		}

		inline void reserve(size_t new_size) noexcept {
			this->resize(new_size);
		}

		inline size_t get_capacity() noexcept {
			return this->capacity;
		}

		inline ~memory_core() noexcept {
			if (data && capacity > 0) {
				allocator.deallocate(data, capacity);
			}
		};

	protected:
		NO_UNIQUE_ADDRESS sentinel_allocator allocator{};
		sentinel_holder* data{};
		size_type capacity{};
		size_type size{};

		inline void emplace_internal(value_type&& value, uint64_t recursion_limit = 1000) noexcept {
			if (is_full()) {
				resize(round_up_to_cache_line(capacity * 4), recursion_limit);
			}
			size_type index = key_hasher{}(key_accessor{}(value)) % capacity;
			size_type current_index = index;
			bool inserted = false;
			while (!inserted) {
				if (!data[current_index].operator bool()) {
					data[current_index].activate(std::forward<value_type>(value));
					size++;
					inserted = true;
				}
				else if (key_accessor{}(data[current_index].operator reference()) == key_accessor{}(value)) {
					data[current_index].activate(std::forward<value_type>(value));
					inserted = true;
				}
				else {
					current_index = (current_index + 1) % capacity;
					if (current_index == index) {
						resize(round_up_to_cache_line(capacity * 4), recursion_limit);
						emplace_internal(std::forward<value_type>(value), recursion_limit);
						return;
					}
				}
			}
		}

		inline void emplace_internal(const value_type& value, uint64_t recursion_limit = 1000) noexcept {
			if (is_full()) {
				resize(round_up_to_cache_line(capacity * 4), recursion_limit);
			}
			size_type index = key_hasher{}(key_accessor{}(value)) % capacity;
			size_type current_index = index;
			bool inserted = false;
			value_type new_element{ value };
			while (!inserted) {
				if (!data[current_index].operator bool()) {
					data[current_index].activate(std::move(new_element));
					size++;
					inserted = true;
				}
				else if (key_accessor{}(data[current_index].operator reference()) == key_accessor{}(value)) {
					data[current_index].activate(std::move(new_element));
					inserted = true;
				}
				else {
					current_index = (current_index + 1) % capacity;
					if (current_index == index) {
						resize(round_up_to_cache_line(capacity * 4), recursion_limit);
						emplace_internal(std::forward<value_type>(value), recursion_limit);
						return;
					}
				}
			}
		}

		inline void resize(size_type new_capacity, uint64_t recursion_limit = 1000) {
			--recursion_limit;
			if (recursion_limit == 0) {
				throw std::runtime_error{ "Sorry, but the max number of recursive resizes has been exceeded." };
			}
			memory_core<key_type, value_type> newData{ new_capacity };
			for (size_type x = 0; x < capacity; x++) {
				if (data[x].operator bool()) {
					newData.emplace_internal(data[x].disable(), recursion_limit);
				}
			}
			std::swap(data, newData.data);
			std::swap(capacity, newData.capacity);
		}

		size_type round_up_to_cache_line(size_type size) {
			const size_type multiple = 64 / sizeof(void*);
			return (size + multiple - 1) / multiple * multiple;
		}
	};

	template<typename KTy, typename OTy, typename KATy = key_accessor<KTy, OTy>> class unordered_set {
	public:
		using key_type = KTy;
		using value_type = OTy;
		using reference = value_type&;
		using const_reference = const reference;
		using pointer = value_type*;
		using key_accessor = KATy;
		using size_type = size_t;
		using iterator = typename memory_core<key_type, value_type>::memory_core_iterator;

		inline unordered_set() : data{ 5 } {};

		inline unordered_set& operator=(unordered_set&& other) noexcept = default;
		inline unordered_set(unordered_set&& other) noexcept = default;
		inline unordered_set& operator=(const unordered_set& other) noexcept = default;
		inline unordered_set(const unordered_set& other) noexcept = default;

		inline void emplace(value_type&& value) noexcept {
			data.emplace(std::forward<value_type>(value));
		}

		inline void emplace(const value_type& value) noexcept {
			data.emplace(value);
		}

		inline bool contains(key_type&& key) noexcept {
			return data.contains(std::forward<key_type>(key));
		}

		inline bool contains(const key_type& key) noexcept {
			return data.contains(key);
		}

		inline void erase(key_type&& key) noexcept {
			data.erase(std::forward<key_type>(key));
		}

		inline void erase(const key_type& key) noexcept {
			data.erase(key);
		}

		inline iterator find(key_type&& key) noexcept {
			return data.find(std::forward<key_type>(key));
		}

		inline iterator find(const key_type& key) noexcept {
			return data.find(key);
		}

		inline reference operator[](key_type&& key) noexcept {
			return *data.find(std::forward<key_type>(key));
		}

		inline reference operator[](const key_type& key) noexcept {
			return *data.find(key);
		}

		inline iterator begin() const noexcept {
			return iterator(data.begin());
		}

		inline iterator end() const noexcept {
			return iterator(data.end());
		}

		inline iterator begin() noexcept {
			return iterator(data.begin());
		}

		inline iterator end() noexcept {
			return iterator(data.end());
		}

		inline size_type size() const noexcept {
			return data.get_size();
		}

		inline void reserve(size_t new_size) noexcept {
			data.reserve(new_size);
		}

		inline bool empty() const noexcept {
			return data.is_empty();
		}

		inline size_type capacity() const noexcept {
			return data.get_capacity();
		}

	protected:
		memory_core<key_type, value_type> data{};
	};

}// namespace DiscordCoreAPI