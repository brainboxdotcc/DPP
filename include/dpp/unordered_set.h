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

#include <shared_mutex>

#ifdef max
#undef max
#endif

namespace dpp {

	template<typename OTy = void> struct fnv1a_hash {
		inline uint64_t operator()(const OTy& data) const;

		inline uint64_t operator()(OTy& data) const;

	protected:
		static constexpr uint64_t fnvOffsetBasis{ 14695981039346656037ull };
		static constexpr uint64_t fnvPrime{ 1099511628211ull };

		inline uint64_t internal_hash_function(const uint8_t* value, size_t count) const;
	};

	template<typename OTy, typename KTy> struct KeyAccessor {
		KTy& operator()(const OTy& other) {
			return other->id;
		}

		KTy& operator()(OTy&& other) {
			return other->id;
		}
	};

	template<typename OTy> class object_allocator {
	public:
		using value_type = OTy;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type*;
		using propagate_on_container_move_assignment = std::true_type;

		template<typename U> struct rebind {
			using other = object_allocator<U>;
		};

		inline object_allocator() noexcept {};

		template<typename U> inline object_allocator(const object_allocator<U>&) noexcept {};

		inline pointer allocate(size_type n) {
			if (n > std::numeric_limits<size_type>::max() / sizeof(value_type)) {
				throw std::bad_alloc();
			}
			pointer p = static_cast<pointer>(operator new[](n * sizeof(value_type)));
			return p;
		}

		inline void deallocate(pointer p, size_type n) noexcept {
			operator delete[](p);
		}

		template<typename... Args> inline void construct(value_type* p, Args&&... args) noexcept {
			new (static_cast<void*>(p)) value_type(std::forward<Args>(args)...);
		}

		inline void destroy(value_type* p) noexcept {
			p->~value_type();
		}
	};

	template<typename OTy, typename KTy, typename KATy = KeyAccessor<OTy, KTy>> class memory_core {
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

			inline memory_core_iterator(memory_core* core, std::size_t index) : core(core), index(index) {
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

		private:
			memory_core* core;
			std::size_t index;
		};

		using iterator = memory_core_iterator;

		class sentinel_holder {
		public:
			inline sentinel_holder() noexcept = default;

			inline sentinel_holder& operator=(sentinel_holder&& other) noexcept = delete;
			inline sentinel_holder(sentinel_holder&& other) noexcept = delete;
			inline sentinel_holder& operator=(const sentinel_holder& other) = delete;
			inline sentinel_holder(const sentinel_holder& other) = delete;

			inline operator bool() const noexcept {
				return isItActive;
			}

			inline operator reference() noexcept {
				return object;
			}

			inline void activate(value_type&& data) noexcept {
				object = std::forward<value_type>(data);
				isItActive = true;
			}

			inline value_type&& disable() noexcept {
				isItActive = false;
				return std::move(object);
			}

		protected:
			bool isItActive{ false };
			value_type object{};
		};

		using sentinel_allocator = object_allocator<sentinel_holder>;

		inline memory_core(size_type capacityNew) : capacity(capacityNew), size(0), data(sentinel_allocator{}.allocate(capacityNew)) {
			for (size_t x = 0; x < capacityNew; ++x) {
				sentinel_allocator{}.construct(&data[x]);
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
				memory_core<value_type, key_type> newData{ other.capacity };
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

		inline void emplace(value_type&& element) noexcept {
			emplace_internal(std::forward<value_type>(element));
		}

		inline void emplace(const value_type& element) noexcept {
			emplace_internal(element);
		}

		inline iterator find(key_type&& key) noexcept {
			size_type index = key_hasher{}(key) % capacity;
			size_type currentIndex{};
			while (currentIndex < capacity) {
				if (data[index].operator bool() && key_accessor {}(data[index].operator reference()) == key) {
					return iterator{ this, index };
				}
				index = (index + 1) % capacity;
				++currentIndex;
			}

			return end();
		}

		inline iterator find(const key_type& key) noexcept {
			size_type index = key_hasher{}(key) % capacity;
			size_type currentIndex{};
			while (currentIndex < capacity) {
				if (data[index].operator bool() && key_accessor {}(data[index].operator reference()) == key) {
					return iterator{ this, index };
				}
				index = (index + 1) % capacity;
				++currentIndex;
			}

			return end();
		}

		inline bool contains(key_type&& key) noexcept {
			size_type index = key_hasher{}(key) % capacity;
			size_type currentIndex{};
			while (currentIndex < capacity) {
				if (data[index].operator bool() && key_accessor {}(data[index].operator reference()) == key) {
					return true;
				}
				index = (index + 1) % capacity;
				++currentIndex;
			}

			return false;
		}

		inline bool contains(const key_type& key) noexcept {
			size_type index = key_hasher{}(key) % capacity;
			size_type currentIndex{};
			while (currentIndex < capacity) {
				if (data[index].operator bool() && key_accessor {}(data[index].operator reference()) == key) {
					return true;
				}
				index = (index + 1) % capacity;
				++currentIndex;
			}

			return false;
		}

		inline void erase(key_type&& key) noexcept {
			size_type index = key_hasher{}(key) % capacity;
			size_type currentIndex{};
			while (currentIndex < capacity) {
				if (data[index].operator bool() && key_accessor {}(data[index].operator reference()) == key) {
					data[index].disable();
					size--;
					if (size < capacity / 4 && capacity > 10) {
						resize(capacity / 2);
					}
					return;
				}
				index = (index + 1) % capacity;
				++currentIndex;
			}
		}

		inline void erase(const key_type& key) noexcept {
			size_type index = key_hasher{}(key) % capacity;
			size_type currentIndex{};
			while (currentIndex < capacity) {
				if (data[index].operator bool() && key_accessor {}(data[index].operator reference()) == key) {
					data[index].disable();
					size--;
					if (size < capacity / 4 && capacity > 10) {
						resize(capacity / 2);
					}
					return;
				}
				index = (index + 1) % capacity;
				++currentIndex;
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

		inline size_type getSize() const noexcept {
			return size;
		}

		inline bool isEmpty() const noexcept {
			return size == 0;
		}

		inline bool isFull() const noexcept {
			return static_cast<float>(size) >= static_cast<float>(capacity) * 0.75f;
		}

		inline void reserve(size_t newSize) noexcept {
			this->resize(newSize);
		}

		inline size_t get_capacity() noexcept {
			return this->capacity;
		}

		inline ~memory_core() noexcept {
			if (data && capacity > 0) {
				sentinel_allocator{}.deallocate(data, capacity);
			}
		};

	protected:
		static constexpr size_type cacheLineSize{ 64 };
		sentinel_holder* data{};
		size_type capacity{};
		size_type size{};

		inline void emplace_internal(value_type&& element, uint64_t recursionLimit = 1000) noexcept {
			if (isFull()) {
				resize(round_up_to_cache_line(capacity * 4), recursionLimit);
			}
			size_type index = key_hasher{}(key_accessor{}(element)) % capacity;
			size_type currentIndex = index;
			bool inserted = false;
			while (!inserted) {
				if (!data[currentIndex].operator bool() || key_accessor{}(data[currentIndex].operator reference()) == key_accessor{}(element)) {
					data[currentIndex].activate(std::forward<value_type>(element));
					size++;
					inserted = true;
				}
				else {
					currentIndex = (currentIndex + 1) % capacity;
					if (currentIndex == index) {
						resize(round_up_to_cache_line(capacity * 4), recursionLimit);
						emplace_internal(std::forward<value_type>(element), recursionLimit);
						return;
					}
				}
			}
		}

		inline void emplace_internal(const value_type& element, uint64_t recursionLimit = 1000) noexcept {
			if (isFull()) {
				resize(round_up_to_cache_line(capacity * 4), recursionLimit);
			}
			size_type index = key_hasher{}(key_accessor{}(element)) % capacity;
			size_type currentIndex = index;
			bool inserted = false;
			value_type newElement{ element };
			while (!inserted) {
				if (!data[currentIndex].operator bool() || key_accessor{}(data[currentIndex].operator reference()) == key_accessor{}(element)) {
					data[currentIndex].activate(std::move(newElement));
					size++;
					inserted = true;
				}
				else {
					currentIndex = (currentIndex + 1) % capacity;
					if (currentIndex == index) {
						resize(round_up_to_cache_line(capacity * 4), recursionLimit);
						emplace_internal(std::move(newElement), recursionLimit);
						return;
					}
				}
			}
		}

		inline void resize(size_type newCapacity, uint64_t recursionLimit = 1000) {
			--recursionLimit;
			if (recursionLimit == 0) {
				throw std::runtime_error{ "Sorry, but the max number of recursive resizes have been exceeded." };
			}
			memory_core<value_type, key_type> newData{ newCapacity };
			for (size_type x = 0; x < capacity; x++) {
				if (data[x].operator bool()) {
					newData.emplace_internal(data[x].disable(), recursionLimit);
				}
			}
			std::swap(data, newData.data);
			std::swap(capacity, newData.capacity);
		}

		size_type round_up_to_cache_line(size_type size) {
			const size_type multiple = cacheLineSize / sizeof(void*);
			return (size + multiple - 1) / multiple * multiple;
		}
	};

	template<typename KTy, typename OTy, typename KATy = KeyAccessor<OTy, KTy>> class unordered_set {
	public:
		using key_type = KTy;
		using value_type = OTy;
		using reference = value_type&;
		using const_reference = const reference;
		using pointer = value_type*;
		using key_accessor = KATy;
		using size_type = size_t;
		using hasher = fnv1a_hash<value_type>;
		using iterator = typename memory_core<value_type, key_type>::memory_core_iterator;

		inline unordered_set() : data{ 5 } {};

		inline unordered_set& operator=(unordered_set&& other) noexcept {
			if (this != &other) {
				std::swap(this->data, other.data);
			}
			return *this;
		}

		inline unordered_set(unordered_set&& other) noexcept {
			*this = std::move(other);
		}

		inline unordered_set& operator=(const unordered_set& other) noexcept {
			if (this != &other) {
				this->data = other.data;
			}
			return *this;
		}

		inline unordered_set(const unordered_set& other) noexcept {
			*this = other;
		}

		inline iterator begin() noexcept {
			return iterator(data.begin());
		}

		inline iterator end() noexcept {
			return iterator(data.end());
		}

		inline void emplace(value_type&& element) noexcept {
			data.emplace(std::forward<value_type>(element));
		}

		inline void emplace(const value_type& element) noexcept {
			data.emplace(element);
		}

		inline bool contains(key_type element) noexcept {
			return data.contains(element);
		}

		inline void erase(key_type element) noexcept {
			data.erase(element);
		}

		inline iterator find(key_type&& dataToFind) noexcept {
			return data.find(dataToFind);
		}

		inline iterator find(key_type dataToFind) noexcept {
			return data.find(dataToFind);
		}

		inline reference operator[](key_type dataToFind) noexcept {
			return *data.find(dataToFind);
		}

		inline size_type size() const noexcept {
			return data.getSize();
		}

		inline void reserve(size_t newSize) noexcept {
			data.reserve(newSize);
		}

		inline bool empty() const noexcept {
			return data.isEmpty();
		}

		inline size_type capacity() const noexcept {
			return data.get_capacity();
		}

	protected:
		memory_core<value_type, key_type> data{};
	};

	template<typename T> class NonUniquePtr;

}// namespace DiscordCoreAPI
