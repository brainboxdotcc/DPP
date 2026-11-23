#pragma once
#include <unordered_map>
#include <shared_mutex>
#include <mutex>
#include <utility>
#include <functional>
#include <optional>

namespace dpp {

/**
 * @brief An unordered map supporting synchronised access from multiple threads.
 * @details Intended for shared mutable caches where concurrent reads and writes
 * would otherwise cause data races and undefined behaviour.
 *
 * Inherits from std::unordered_map for API compatibility. Inherited methods
 * are not individually thread-safe; callers must use read() or write() when
 * accessing the map concurrently.
 *
 * @tparam Key Key type
 * @tparam T Mapped value type
 * @tparam Hash Hash function
 * @tparam KeyEqual Key comparison function
 * @tparam Allocator Allocator type
 */
template<typename Key, typename T, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>, typename Allocator = std::allocator<std::pair<const Key, T>>>
class thread_safe_unordered_map : public std::unordered_map<Key, T, Hash, KeyEqual, Allocator> {
    using base = std::unordered_map<Key, T, Hash, KeyEqual, Allocator>;
    mutable std::shared_mutex mutex;

public:
    using base::base;

    /**
     * @brief Construct an empty thread-safe unordered map.
     */
    thread_safe_unordered_map() = default;

    /**
     * @brief Copy construct a thread-safe unordered map.
     * @param other Map to copy
     */
    thread_safe_unordered_map(const thread_safe_unordered_map& other) : base(other.read([](const base& map) { return map; })) {}

    /**
     * @brief Move construct a thread-safe unordered map.
     * @param other Map to move from
     */
    thread_safe_unordered_map(thread_safe_unordered_map&& other) : base(other.write([](base& map) { return std::move(map); })) {}

    /**
     * @brief Copy assign a thread-safe unordered map.
     * @param other Map to copy
     * @return Reference to this map
     */
    thread_safe_unordered_map& operator=(const thread_safe_unordered_map& other) {
        if (this != &other) {
            std::unique_lock dest(mutex, std::defer_lock);
            std::shared_lock<std::shared_mutex> source(other.mutex, std::defer_lock);
            std::lock(dest, source);
            base::operator=(static_cast<const base&>(other));
        }
        return *this;
    }

    /**
     * @brief Move assign a thread-safe unordered map.
     * @param other Map to move from
     * @return Reference to this map
     */
    thread_safe_unordered_map& operator=(thread_safe_unordered_map&& other) {
        if (this != &other) {
            std::scoped_lock lock(mutex, other.mutex);
            base::operator=(std::move(static_cast<base&>(other)));
        }
        return *this;
    }

    /**
     * @brief Safely access the map for reading.
     * @details Allows concurrent readers while preventing modification.
     * Do not retain references or iterators beyond the callback.
     *
     * @tparam F Callable type
     * @param fn Function receiving a const reference to the map
     * @return The result of the callable
     */
    template<typename F>
    decltype(auto) read(F&& fn) const {
        std::shared_lock lock(mutex);
        return std::forward<F>(fn)(static_cast<const base&>(*this));
    }

    /**
     * @brief Safely retrieve a copy of a value by key.
     * @details Returns a snapshot of the mapped value while holding a shared lock.
     * Modifying the returned value does not modify the map.
     *
     * @param key Key to look up
     * @return A copy of the value, or std::nullopt if the key does not exist
     */
    std::optional<T> get(const Key& key) const {
        std::shared_lock lock(mutex);
        auto it = base::find(key);
        if (it != base::end()) {
            return it->second;
        }
        return std::nullopt;
    }

    /**
     * @brief Safely access the map for modification.
     * @details Provides exclusive access for compound operations.
     * Do not dispatch events, queue work or invoke user callbacks while
     * holding the lock.
     *
     * @tparam F Callable type
     * @param fn Function receiving a mutable reference to the map
     * @return The result of the callable
     */
    template<typename F>
    decltype(auto) write(F&& fn) {
        std::unique_lock lock(mutex);
        return std::forward<F>(fn)(static_cast<base&>(*this));
    }
};

}