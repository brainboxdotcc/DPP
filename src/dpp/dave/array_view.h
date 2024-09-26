#pragma once

#include <cassert>
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

